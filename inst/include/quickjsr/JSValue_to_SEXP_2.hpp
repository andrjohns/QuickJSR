#ifndef QUICKJSR_JSVALUE_TO_SEXP_2_HPP
#define QUICKJSR_JSVALUE_TO_SEXP_2_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <quickjsr/JSValue_to_Cpp.hpp>

namespace quickjsr {

SEXP JSValue_to_SEXP_2(JSContext* ctx, const JSValue& val) {
  switch (JS_VALUE_GET_TAG(val)) {
    case JS_TAG_NULL: {
      return R_NilValue;
    }
    case JS_TAG_UNDEFINED: {
      return R_NilValue;
    }
    case JS_TAG_BOOL: {
      return cpp11::as_sexp(JSValue_to_Cpp<bool>(ctx, val));
    }
    case JS_TAG_INT: {
      return cpp11::as_sexp(JSValue_to_Cpp<int32_t>(ctx, val));
    }
    case JS_TAG_FLOAT64: {
      return cpp11::as_sexp(JSValue_to_Cpp<double>(ctx, val));
    }
    case JS_TAG_STRING: {
      return cpp11::as_sexp(JSValue_to_Cpp<std::string>(ctx, val));
    }
    case JS_TAG_BIG_INT: {
      return cpp11::as_sexp(JSValue_to_Cpp<int64_t>(ctx, val));
    }
    case JS_TAG_SHORT_BIG_INT: {
      return cpp11::as_sexp(JSValue_to_Cpp<int64_t>(ctx, val));
    }
    case JS_TAG_OBJECT: {
      if (JS_IsArray(val)) {
        // Handle as array
        int64_t len;
        JS_GetLength(ctx, val, &len);
        cpp11::writable::list out(len);
        for (int64_t i = 0; i < len; i++) {
          JSValue elem = JS_GetPropertyInt64(ctx, val, i);
          out[static_cast<R_xlen_t>(i)] = JSValue_to_SEXP_2(ctx, elem);
          JS_FreeValue(ctx, elem);
        }
        return out;
      } else {
        // Handle as object
        // Get the keys of the object
        JSPropertyEnum* tab = NULL;
        uint32_t len = 0;
        JS_GetOwnPropertyNames(ctx, &tab, &len, val, JS_GPN_STRING_MASK);
        cpp11::writable::strings keys(len);
        cpp11::writable::list out(len);
        for (uint32_t i = 0; i < len; i++) {
          JSValue elem = JS_GetProperty(ctx, val, tab[i].atom);
          out[static_cast<R_xlen_t>(i)] = JSValue_to_SEXP_2(ctx, elem);

          const char* key = JS_AtomToCString(ctx, tab[i].atom);
          keys[static_cast<R_xlen_t>(i)] = key;

          JS_FreeValue(ctx, elem);
          JS_FreeCString(ctx, key);
        }
        JS_FreePropertyEnum(ctx, tab, len);
        out.attr("names") = keys;
        return out;
      }
    }
  }
}

} // namespace quickjsr

#endif
