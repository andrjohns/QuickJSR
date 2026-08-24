#ifndef QUICKJSR_JS_SEXP_HPP
#define QUICKJSR_JS_SEXP_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <quickjsr/JSValue_to_SEXP.hpp>

// Need to redefine the JS_CFUNC_DEF macro as it uses C features
// (designated initializers) which are not supported in C++ (until C++20)
#define JS_CFUNC_DEF_CPP(name, length, func1) { \
  name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0, \
  { { length, JS_CFUNC_generic, { func1 } } } \
  }

#ifndef countof
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif

namespace quickjsr {
  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox,
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

  static JSValue js_r_package(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc != 1) {
      return JS_ThrowTypeError(ctx, "R.package requires one argument");
    }

    const char *package_name = JS_ToCString(ctx, argv[0]);
    if (!package_name) {
        return JS_EXCEPTION;
    }
    SEXP pkg_ns;
    if (strcmp(package_name, "base") == 0) {
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
    JS_SetPropertyFunctionList(ctx, r_obj, js_r_funcs, countof(js_r_funcs));
    return r_obj;
  }
}

#endif
