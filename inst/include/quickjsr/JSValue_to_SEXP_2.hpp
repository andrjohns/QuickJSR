#ifndef QUICKJSR_JSVALUE_TO_SEXP_2_HPP
#define QUICKJSR_JSVALUE_TO_SEXP_2_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <quickjsr/JSValue_to_Cpp.hpp>
#include <iostream>

namespace quickjsr {

  enum BaseType {
    Number,
    String,
    Boolean,
    DateNew,
    Null,
    Array,
    ObjectNew,
    Mixed,
    Error
  };



  BaseType value_to_base_type(const JSValue& value) {
    if (JS_IsException(value)) {
      return Error;
    }
    if (JS_IsNull(value) || JS_IsUndefined(value) || JS_IsUninitialized(value)) {
      return Null;
    }
    if (JS_IsBool(value)) {
      return Boolean;
    }
    if (JS_IsString(value)) {
      return String;
    }
    if (JS_IsDate(value)) {
      return DateNew;
    }
    if (JS_IsNumber(value)) {
      return Number;
    }
    if (JS_IsArray(value)) {
      return Array;
    }
    if (JS_IsObject(value)) {
      return ObjectNew;
    }
    return Mixed;
  }

  BaseType combine_array_types(BaseType a, BaseType b) {
    if (a == Mixed || b == Mixed) {
      return Mixed;
    }
    if (a == ObjectNew || b == ObjectNew) {
      return Mixed;
    }
    if (a == Array || b == Array) {
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
  switch (value_to_base_type(val)) {
    case Error: {
      JSValue exc = JS_GetException(ctx);
      std::string msg = JSValue_to_Cpp<std::string>(ctx, exc);
      JS_FreeValue(ctx, exc);
      cpp11::stop("JavaScript Exception: " + msg);
    }
    case Null: {
      return R_NilValue;
    }
    case Boolean: {
      return cpp11::as_sexp(JSValue_to_Cpp<bool>(ctx, val));
    }
    case Number: {
      return cpp11::as_sexp(JSValue_to_Cpp<double>(ctx, val));
    }
    case String: {
      return cpp11::as_sexp(JSValue_to_Cpp<std::string>(ctx, val));
    }
    case DateNew: {
      double time_ms;
      JSValue time_val = JS_GetTime(ctx, val);
      JS_ToFloat64(ctx, &time_ms, time_val);
      JS_FreeValue(ctx, time_val);
      double time_s = time_ms / 1000.0; // Convert milliseconds to seconds
      cpp11::writable::doubles out(1);
      out[0] = time_s;
      out.attr("class") = cpp11::writable::strings({"POSIXct", "POSIXt"});
      out.attr("tzone") = "UTC";
      return out;
    }
    case Array: {
      // Handle as array
      int64_t len;
      JS_GetLength(ctx, val, &len);

      JSValue base_val = JS_GetPropertyInt64(ctx, val, 0);
      BaseType prev_type = value_to_base_type(base_val);
      JS_FreeValue(ctx, base_val);

      for (int64_t i = 1; i < len; i++) {
        base_val = JS_GetPropertyInt64(ctx, val, i);
        BaseType curr_type = value_to_base_type(base_val);
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
        bool all_double = true;
        bool all_same_size = true;
        int64_t first_size = -1;
        for (int64_t i = 0; i < len; i++) {
          JSValue elem = JS_GetPropertyInt64(ctx, val, i);
          SEXP elem_sexp = JSValue_to_SEXP_2(ctx, elem);
          if (all_double && all_same_size) {
            if (TYPEOF(elem_sexp) != REALSXP) {
              all_double = false;
            }
            R_xlen_t elem_size = Rf_xlength(elem_sexp);
            if (first_size == -1) {
              first_size = elem_size;
            } else if (elem_size != first_size) {
              all_same_size = false;
            }
          }

          out[static_cast<R_xlen_t>(i)] = elem_sexp;
          JS_FreeValue(ctx, elem);
        }

        if (all_double && all_same_size && first_size > 1) {
          cpp11::function unlist = cpp11::package("base")["unlist"];
          cpp11::function matrix = cpp11::package("base")["matrix"];
          return matrix(unlist(out), len, first_size, true);
        } else {
          return out;
        }
      }
    }
    case ObjectNew: {
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
    default: {
      return R_NilValue;
    }
  }
  return R_NilValue; // Fallback for unhandled types
}

} // namespace quickjsr

#endif
