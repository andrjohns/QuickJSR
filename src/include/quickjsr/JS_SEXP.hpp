#ifndef QUICKJSR_JS_SEXP_HPP
#define QUICKJSR_JS_SEXP_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <quickjsr/JSValue_to_SEXP.hpp>
#include <quickjsr/MaskedTypedArray.hpp>
#include <quickjsr/RVectorView.hpp>
#include <climits>
#include <cmath>
#include <iterator>
#include <string_view>

extern "C" int quickjsr_atom_to_array_index(JSContext* ctx, JSAtom atom,
                                               uint32_t* index);

// Need to redefine the JS_CFUNC_DEF macro as it uses C features
// (designated initializers) which are not supported in C++ (until C++20)
#define JS_CFUNC_DEF_CPP(name, length, func1) { \
  name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0, \
  { { length, JS_CFUNC_generic, { func1 } } } \
  }

namespace quickjsr {
  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox,
                                  bool auto_unbox_curr);
  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox,
                                  bool auto_unbox_curr, int64_t index,
                                  bool is_factor, bool is_date_class);
  inline JSValue SEXP_to_JSValue_array(JSContext* ctx, const SEXP& x,
                                        bool auto_unbox,
                                        bool auto_unbox_curr);

  static JSClassID js_sexp_class_id;
  static JSClassID js_renv_class_id;

  // The wrapped SEXP (an R closure, see SEXP_to_JSValue_function) is kept
  // alive with R_PreserveObject() for as long as this JS object holds a raw
  // pointer to it, since nothing else protects it from R's GC once the
  // .Call() that produced it returns.
  static void js_sexp_finalizer(JSRuntime *rt, JSValueConst val) {
    SEXP x = reinterpret_cast<SEXP>(JS_GetOpaque(val, js_sexp_class_id));
    if (x) {
      R_ReleaseObject(x);
    }
  }

  static JSClassDef js_sexp_class_def = {
    "SEXP",
    js_sexp_finalizer
  };

  static JSValue js_renv_get_property(JSContext *ctx, JSValueConst this_val, JSAtom atom, JSValueConst receiver) {
    const char *property_name = JS_AtomToCString(ctx, atom);
    SEXP x = reinterpret_cast<SEXP>(JS_GetOpaque(this_val, js_renv_class_id));
    if (!property_name) {
      return JS_EXCEPTION;
    }
    if (!cpp11::detail::r_env_has(x, Rf_install(property_name))) {
      JS_FreeCString(ctx, property_name);
      return JS_UNDEFINED;
    }
    SEXP name = PROTECT(Rf_mkString(property_name));
    SEXP call = PROTECT(Rf_lang3(Rf_install("[["), x, name));
    int error = 0;
    SEXP result = R_tryEvalSilent(call, R_BaseEnv, &error);
    if (error) {
      std::string message = R_curErrorBuf();
      UNPROTECT(2);
      JS_FreeCString(ctx, property_name);
      return JS_ThrowPlainError(ctx, "%s", message.c_str());
    }
    PROTECT(result);
    JSValue value = SEXP_to_JSValue(ctx, result, true, true);
    UNPROTECT(3);
    JS_FreeCString(ctx, property_name);
    return value;
  }

  static int js_renv_set_property(JSContext *ctx, JSValueConst this_val, JSAtom atom, JSValueConst value, JSValueConst receiver, int flags) {
    const char *property_name = JS_AtomToCString(ctx, atom);
    if (!property_name) {
      return -1;
    }
    SEXP x = reinterpret_cast<SEXP>(JS_GetOpaque(this_val, js_renv_class_id));
    SEXP converted = PROTECT(JSValue_to_SEXP(ctx, value));
    SEXP name = PROTECT(Rf_mkString(property_name));
    SEXP call = PROTECT(Rf_lang4(Rf_install("[[<-"), x, name, converted));
    int error = 0;
    R_tryEvalSilent(call, R_BaseEnv, &error);
    if (error) {
      std::string message = R_curErrorBuf();
      UNPROTECT(3);
      JS_FreeCString(ctx, property_name);
      JS_ThrowPlainError(ctx, "%s", message.c_str());
      return -1;
    }
    UNPROTECT(3);
    JS_FreeCString(ctx, property_name);
    return 0;
  }

  static JSClassExoticMethods js_renv_exotic_methods = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    js_renv_get_property,
    js_renv_set_property
  };

  static void js_renv_finalizer(JSRuntime *rt, JSValueConst val) {
    SEXP x = reinterpret_cast<SEXP>(JS_GetOpaque(val, js_renv_class_id));
    if (x) {
      R_ReleaseObject(x);
    }
  }

  static JSClassDef js_renv_class_def = {
    "REnv",
    js_renv_finalizer,
    nullptr,
    nullptr,
    &js_renv_exotic_methods
  };

  static void js_rvector_view_finalizer(JSRuntime* rt, JSValueConst val) {
    auto* data = static_cast<RVectorViewData*>(
      JS_GetOpaque(val, js_rvector_view_class_id)
    );
    if (data) {
      R_ReleaseObject(data->source);
      delete data;
    }
  }

  static int js_rvector_view_get_own_property(
    JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst obj, JSAtom prop
  ) {
    uint32_t index;
    if (!quickjsr_atom_to_array_index(ctx, prop, &index)) return 0;
    auto* data = static_cast<RVectorViewData*>(
      JS_GetOpaque2(ctx, obj, js_rvector_view_class_id)
    );
    if (!data) return -1;
    if (index >= static_cast<uint64_t>(Rf_xlength(data->value))) return 0;
    if (desc) {
      desc->flags = JS_PROP_ENUMERABLE;
      if (data->mutable_view) desc->flags |= JS_PROP_WRITABLE;
      desc->value = SEXP_to_JSValue(
        ctx, data->value, true, true, index, data->factor, data->date
      );
      desc->getter = JS_UNDEFINED;
      desc->setter = JS_UNDEFINED;
      if (JS_IsException(desc->value)) return -1;
    }
    return 1;
  }

  static int js_rvector_view_delete_property(
    JSContext* ctx, JSValueConst obj, JSAtom prop
  ) {
    uint32_t index;
    if (!quickjsr_atom_to_array_index(ctx, prop, &index)) return 1;
    auto* data = static_cast<RVectorViewData*>(
      JS_GetOpaque2(ctx, obj, js_rvector_view_class_id)
    );
    if (!data) return -1;
    if (index >= static_cast<uint64_t>(Rf_xlength(data->value))) return 1;
    JS_ThrowTypeError(
      ctx,
      data->mutable_view ? "mutable R vector views have fixed length" :
                           "R vector views are read-only"
    );
    return -1;
  }

  static int js_rvector_view_get_own_property_names(
    JSContext* ctx, JSPropertyEnum** properties, uint32_t* count,
    JSValueConst obj
  ) {
    auto* data = static_cast<RVectorViewData*>(
      JS_GetOpaque2(ctx, obj, js_rvector_view_class_id)
    );
    if (!data) return -1;
    R_xlen_t size = Rf_xlength(data->value);
    if (size > UINT32_MAX) {
      JS_ThrowRangeError(ctx, "R vector view is too long to enumerate");
      return -1;
    }
    if (size == 0) {
      *properties = nullptr;
      *count = 0;
      return 0;
    }
    auto* result = static_cast<JSPropertyEnum*>(
      js_malloc(ctx, static_cast<size_t>(size) * sizeof(JSPropertyEnum))
    );
    if (!result) return -1;
    for (uint32_t i = 0; i < static_cast<uint32_t>(size); i++) {
      result[i].is_enumerable = true;
      result[i].atom = JS_NewAtomUInt32(ctx, i);
      if (result[i].atom == JS_ATOM_NULL) {
        for (uint32_t j = 0; j < i; j++) JS_FreeAtom(ctx, result[j].atom);
        js_free(ctx, result);
        return -1;
      }
    }
    *properties = result;
    *count = static_cast<uint32_t>(size);
    return 0;
  }

  static int js_rvector_view_set_property(
    JSContext* ctx, JSValueConst obj, JSAtom prop, JSValueConst value,
    JSValueConst receiver, int flags
  ) {
    auto* data = static_cast<RVectorViewData*>(
      JS_GetOpaque2(ctx, obj, js_rvector_view_class_id)
    );
    if (!data) return -1;
    if (!data->mutable_view) {
      JS_ThrowTypeError(ctx, "R vector views are read-only");
      return -1;
    }
    uint32_t index;
    if (!quickjsr_atom_to_array_index(ctx, prop, &index) ||
        index >= static_cast<uint64_t>(Rf_xlength(data->value))) {
      JS_ThrowRangeError(ctx, "mutable R vector view index is out of bounds");
      return -1;
    }
    if (JS_IsNull(value) || JS_IsUndefined(value)) {
      switch (TYPEOF(data->value)) {
        case LGLSXP:
          SET_LOGICAL_ELT(data->value, index, NA_LOGICAL);
          return 1;
        case INTSXP:
          SET_INTEGER_ELT(data->value, index, NA_INTEGER);
          return 1;
        case REALSXP:
          SET_REAL_ELT(data->value, index, NA_REAL);
          return 1;
        case STRSXP:
          SET_STRING_ELT(data->value, index, NA_STRING);
          return 1;
        default:
          JS_ThrowTypeError(ctx, "raw views cannot contain missing values");
          return -1;
      }
    }
    switch (TYPEOF(data->value)) {
      case RAWSXP: {
        double converted;
        if (!JS_IsNumber(value) || JS_ToFloat64(ctx, &converted, value) ||
            !std::isfinite(converted) || std::trunc(converted) != converted ||
            converted < 0 || converted > 255) {
          JS_ThrowTypeError(ctx, "raw view values must be integers from 0 to 255");
          return -1;
        }
#if R_VERSION >= R_Version(4, 2, 0)
        SET_RAW_ELT(data->value, index, static_cast<Rbyte>(converted));
#else
        RAW(data->value)[index] = static_cast<Rbyte>(converted);
#endif
        return 1;
      }
      case LGLSXP:
        if (!JS_IsBool(value)) {
          JS_ThrowTypeError(ctx, "logical view values must be boolean or null");
          return -1;
        }
        SET_LOGICAL_ELT(data->value, index, JS_ToBool(ctx, value));
        return 1;
      case INTSXP: {
        double converted;
        if (!JS_IsNumber(value) || JS_ToFloat64(ctx, &converted, value) ||
            !std::isfinite(converted) || std::trunc(converted) != converted ||
            converted <= INT_MIN || converted > INT_MAX) {
          JS_ThrowTypeError(ctx, "integer view values must be R integers or null");
          return -1;
        }
        SET_INTEGER_ELT(data->value, index, static_cast<int>(converted));
        return 1;
      }
      case REALSXP: {
        double converted;
        if (!JS_IsNumber(value) || JS_ToFloat64(ctx, &converted, value)) {
          JS_ThrowTypeError(ctx, "double view values must be numbers or null");
          return -1;
        }
        SET_REAL_ELT(data->value, index, converted);
        return 1;
      }
      case STRSXP: {
        if (!JS_IsString(value)) {
          JS_ThrowTypeError(ctx, "character view values must be strings or null");
          return -1;
        }
        size_t size;
        const char* converted = JS_ToCStringLen(ctx, &size, value);
        if (!converted) return -1;
        SET_STRING_ELT(
          data->value, index,
          Rf_mkCharLenCE(converted, static_cast<int>(size), CE_UTF8)
        );
        JS_FreeCString(ctx, converted);
        return 1;
      }
      default:
        JS_ThrowTypeError(ctx, "unsupported mutable R vector view type");
        return -1;
    }
  }

  static JSValue js_rvector_view_to_array(
    JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv
  ) {
    auto* data = static_cast<RVectorViewData*>(
      JS_GetOpaque2(ctx, this_val, js_rvector_view_class_id)
    );
    if (!data) return JS_EXCEPTION;
    return SEXP_to_JSValue_array(ctx, data->value, true, true);
  }

  static const JSCFunctionListEntry js_rvector_view_funcs[] = {
    JS_CFUNC_DEF_CPP("toArray", 0, js_rvector_view_to_array),
  };

  static JSClassExoticMethods js_rvector_view_exotic_methods = {
    js_rvector_view_get_own_property,
    js_rvector_view_get_own_property_names,
    js_rvector_view_delete_property,
    nullptr,
    nullptr,
    nullptr,
    js_rvector_view_set_property
  };

  static JSClassDef js_rvector_view_class_def = {
    "RVectorView",
    js_rvector_view_finalizer,
    nullptr,
    nullptr,
    &js_rvector_view_exotic_methods
  };

  static void js_masked_typed_array_finalizer(
    JSRuntime* rt, JSValueConst val
  ) {
    delete static_cast<MaskedTypedArrayData*>(
      JS_GetOpaque(val, js_masked_typed_array_class_id)
    );
  }

  static JSClassDef js_masked_typed_array_class_def = {
    "RMaskedTypedArray",
    js_masked_typed_array_finalizer
  };

  static JSValue js_r_package(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc != 1) {
      return JS_ThrowTypeError(ctx, "R.package requires one argument");
    }

    const char *package_name = JS_ToCString(ctx, argv[0]);
    if (!package_name) {
        return JS_EXCEPTION;
    }
    SEXP pkg_ns;
    if (std::string_view(package_name) == "base") {
      pkg_ns = R_BaseEnv;
    } else {
      pkg_ns = cpp11::detail::r_ns_env(package_name);
      if (pkg_ns == R_NilValue) {
        JSValue exc = JS_ThrowTypeError(ctx, "Can't find namespace '%s' - the package must already be loaded", package_name);
        JS_FreeCString(ctx, package_name);
        return exc;
      }
    }
    JS_FreeCString(ctx, package_name);
    return SEXP_to_JSValue(ctx, pkg_ns, true, true);
  }

  static const JSCFunctionListEntry js_r_funcs[] = {
    JS_CFUNC_DEF_CPP("package", 1, js_r_package),
  };

  static JSValue create_r_object(JSContext *ctx) {
    JSValue r_obj = JS_NewObject(ctx);
    if (JS_IsException(r_obj)) {
      return r_obj;
    }
    JS_SetPropertyFunctionList(
      ctx, r_obj, js_r_funcs, static_cast<int>(std::size(js_r_funcs))
    );
    return r_obj;
  }
}

#endif
