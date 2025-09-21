#ifndef QUICKJSR_JSVALUE_TO_SEXP_2_HPP
#define QUICKJSR_JSVALUE_TO_SEXP_2_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <quickjsr/JSValue_to_Cpp.hpp>
#include <iostream>

namespace quickjsr {

  enum ArrayType {
    Number,
    String,
    Boolean,
    Null,
    Mixed
  };

  ArrayType tag_to_array_type(int tag) {
    switch (tag) {
      case JS_TAG_INT:
        return Number;
      case JS_TAG_BIG_INT:
        return Number;
      case JS_TAG_SHORT_BIG_INT:
        return Number;
      case JS_TAG_FLOAT64:
        return Number;
      case JS_TAG_STRING:
        return String;
      case JS_TAG_BOOL:
        return Boolean;
      case JS_TAG_NULL:
        return Null;
      case JS_TAG_UNDEFINED:
        return Null;
      default:
        return Mixed;
    }
  }

  ArrayType combine_array_types(ArrayType a, ArrayType b) {
    if (a == Mixed || b == Mixed) {
      return Mixed;
    }
    if (a == Null) {
      return b;
    }
    if (b == Null) {
      return a;
    }
    if (a == b) {
      return a;
    }
    if ((a == Number && b == Boolean) || (a == Boolean && b == Number)) {
      return Number;
    }
    return Mixed;
  }

SEXP JSValue_to_SEXP_2(JSContext* ctx, const JSValue& val) {
  switch (JS_VALUE_GET_TAG(val)) {
    case JS_TAG_EXCEPTION: {
      JSValue exc = JS_GetException(ctx);
      std::string msg = JSValue_to_Cpp<std::string>(ctx, exc);
      JS_FreeValue(ctx, exc);
      cpp11::stop("JavaScript Exception: " + msg);
    }
    case JS_TAG_NULL: {
      return R_NilValue;
    }
    case JS_TAG_UNDEFINED: {
      return R_NilValue;
    }
    case JS_TAG_UNINITIALIZED: {
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

        JSValue base_val = JS_GetPropertyInt64(ctx, val, 0);
        ArrayType prev_type = tag_to_array_type(JS_VALUE_GET_TAG(base_val));
        JS_FreeValue(ctx, base_val);

        for (int64_t i = 1; i < len; i++) {
          base_val = JS_GetPropertyInt64(ctx, val, i);
          ArrayType curr_type = tag_to_array_type(JS_VALUE_GET_TAG(base_val));
          JS_FreeValue(ctx, base_val);
          prev_type = combine_array_types(prev_type, curr_type);
          if (prev_type == Mixed) {
            // No need to continue checking types, we know it's mixed
            break;
          }
        }

        if (prev_type == Number) {
          cpp11::writable::doubles out(len);
          for (int64_t i = 0; i < len; i++) {
            base_val = JS_GetPropertyInt64(ctx, val, i);
            if (JS_IsNull(base_val) || JS_IsUndefined(base_val)) {
              out[i] = NA_REAL;
            } else {
              out[i] = JSValue_to_Cpp<double>(ctx, base_val);
            }
            JS_FreeValue(ctx, base_val);
          }
          return out;
        } else if (prev_type == String) {
          cpp11::writable::strings out(len);
          for (int64_t i = 0; i < len; i++) {
            base_val = JS_GetPropertyInt64(ctx, val, i);
            if (JS_IsNull(base_val) || JS_IsUndefined(base_val)) {
              out[i] = NA_STRING;
            } else {
              out[i] = JSValue_to_Cpp<std::string>(ctx, base_val);
            }
            JS_FreeValue(ctx, base_val);
          }
          return out;
        } else if (prev_type == Boolean || prev_type == Null) {
          cpp11::writable::logicals out(len);
          for (int64_t i = 0; i < len; i++) {
            base_val = JS_GetPropertyInt64(ctx, val, i);
            if (JS_IsNull(base_val) || JS_IsUndefined(base_val)) {
              out[i] = NA_LOGICAL;
            } else {
              out[i] = JSValue_to_Cpp<bool>(ctx, base_val);
            }
            JS_FreeValue(ctx, base_val);
          }
          return out;
        } else {
          // Mixed types, return as list
          cpp11::writable::list out(len);
          for (int64_t i = 0; i < len; i++) {
            JSValue elem = JS_GetPropertyInt64(ctx, val, i);
            out[static_cast<R_xlen_t>(i)] = JSValue_to_SEXP_2(ctx, elem);
            JS_FreeValue(ctx, elem);
          }
          return out;
        }
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
    default: {
      std::string type_str = "Unsupported type: ";
      // Get result of typeof
      JSValue typeof_val = JS_GetPropertyStr(ctx, val, "typeof");
      type_str += JSValue_to_Cpp<std::string>(ctx, typeof_val);
      JS_FreeValue(ctx, typeof_val);
      return cpp11::as_sexp(type_str.c_str());
    }
  }
  return R_NilValue; // Fallback for unhandled types
}

} // namespace quickjsr

#endif
