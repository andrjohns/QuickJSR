#ifndef QUICKJSR_JSVALUE_TO_SEXP_HPP
#define QUICKJSR_JSVALUE_TO_SEXP_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <quickjsr/JSValue_to_Cpp.hpp>
#include <quickjsr/JSCommonType.hpp>
#include <quickjsr/JSValue_Date.hpp>

namespace quickjsr {

// Used internally in QuickJS, but not exposed in the public API, so redefine it here
static void js_free_prop_enum(JSContext *ctx, JSPropertyEnum *tab, uint32_t len) {
  uint32_t i;
  if (tab) {
    for (i = 0; i < len; i++) {
      JS_FreeAtom(ctx, tab[i].atom);
    }
    js_free(ctx, tab);
  }
}

// Forward declaration to allow for recursive calls
SEXP JSValue_to_SEXP(JSContext* ctx, const JSValue& val);

SEXP JSValue_to_SEXP_scalar(JSContext* ctx, const JSValue& val) {
  if (JS_IsUndefined(val)) {
    return R_NilValue;
  }
  if (JS_IsBool(val)) {
    return cpp11::as_sexp(JSValue_to_Cpp<bool>(ctx, val));
  }
  if (JS_VALUE_GET_TAG(val) == JS_TAG_INT) {
    return cpp11::as_sexp(JSValue_to_Cpp<int32_t>(ctx, val));
  }
  if (JS_IsNumber(val)) {
    return cpp11::as_sexp(JSValue_to_Cpp<double>(ctx, val));
  }
  if (JS_IsString(val)) {
    return cpp11::as_sexp(JSValue_to_Cpp<std::string>(ctx, val));
  }
  if (JS_IsDate(ctx, val)) {
    cpp11::writable::doubles res = cpp11::as_sexp(JSValue_to_Cpp<double>(ctx, val));
    res.attr("class") = "POSIXct";
    return res;
  }
  return cpp11::as_sexp("Unsupported type");
}

SEXP JSValue_to_SEXP_vector(JSContext* ctx, const JSValue& val) {
  switch (JS_ArrayCommonType(ctx, val)) {
    case Integer:
      return cpp11::as_sexp(JSValue_to_Cpp<std::vector<int>>(ctx, val));
    case Double:
      return cpp11::as_sexp(JSValue_to_Cpp<std::vector<double>>(ctx, val));
    case Logical:
      return cpp11::as_sexp(JSValue_to_Cpp<std::vector<bool>>(ctx, val));
    case Character:
      return cpp11::as_sexp(JSValue_to_Cpp<std::vector<std::string>>(ctx, val));
    case Undefined:
      return R_NilValue;
    case Date: {
      cpp11::writable::doubles res = cpp11::as_sexp(JSValue_to_Cpp<std::vector<double>>(ctx, val));
      res.attr("class") = "POSIXct";
      return res;
    }
    case NumberArray: {
      std::vector<std::vector<double>> res = JSValue_to_Cpp<std::vector<std::vector<double>>>(ctx, val);
      // Check that the inner vectors are all the same length
      size_t len = res[0].size();
      bool allSameSize = std::all_of(res.begin() + 1, res.end(),
                                    [len](auto&& vec) { return vec.size() == len; });
      if (allSameSize) {
        cpp11::writable::doubles_matrix<cpp11::by_column> out(res.size(), len);
        for (size_t i = 0; i < res.size(); i++) {
          for (size_t j = 0; j < len; j++) {
            out(i, j) = res[i][j];
          }
        }
        return out;
      } else {
        cpp11::writable::list out(res.size());
        for (size_t i = 0; i < res.size(); i++) {
          out[static_cast<R_xlen_t>(i)] = cpp11::as_sexp(res[i]);
        }
        return out;
      }
    }
    case Object: {
      uint32_t len;
      JSValue arr_len = JS_GetPropertyStr(ctx, val, "length");
      JS_ToUint32(ctx, &len, arr_len);
      JS_FreeValue(ctx, arr_len);

      cpp11::writable::list out(len);
      for (uint32_t i = 0; i < len; i++) {
        JSValue elem = JS_GetPropertyUint32(ctx, val, i);
        out[static_cast<R_xlen_t>(i)] = JSValue_to_SEXP(ctx, elem);
        JS_FreeValue(ctx, elem);
      }
      return out;
    }
    default: {
      std::string type_str = "Unsupported type: ";
      // Get result of typeof
      JSValue typeof_val = JS_GetPropertyStr(ctx, val, "typeof");
      type_str += JSValue_to_Cpp<std::string>(ctx, typeof_val);
      JS_FreeValue(ctx, typeof_val);
      return cpp11::as_sexp(type_str.c_str());
    }
  }
}

SEXP JSValue_to_SEXP_list(JSContext* ctx, const JSValue& val) {
  // Get the keys of the object
  JSPropertyEnum* tab = NULL;
  uint32_t len = 0;
  JS_GetOwnPropertyNames(ctx, &tab, &len, val, JS_GPN_STRING_MASK);
  cpp11::writable::strings keys(len);
  cpp11::writable::list out(len);
  for (uint32_t i = 0; i < len; i++) {
    JSValue elem = JS_GetProperty(ctx, val, tab[i].atom);
    out[static_cast<R_xlen_t>(i)] = JSValue_to_SEXP(ctx, elem);

    const char* key = JS_AtomToCString(ctx, tab[i].atom);
    keys[static_cast<R_xlen_t>(i)] = key;

    JS_FreeValue(ctx, elem);
    JS_FreeCString(ctx, key);
  }
  js_free_prop_enum(ctx, tab, len);
  out.attr("names") = keys;
  return out;
}

SEXP JSValue_to_SEXP(JSContext* ctx, const JSValue& val) {
  if (JS_IsUndefined(val)) {
    return R_NilValue;
  }
  if (JS_IsArray(ctx, val)) {
    return JSValue_to_SEXP_vector(ctx, val);
  }
  if (JS_IsObject(val)) {
    return JSValue_to_SEXP_list(ctx, val);
  }
  return JSValue_to_SEXP_scalar(ctx, val);
}

} // namespace quickjsr

#endif
