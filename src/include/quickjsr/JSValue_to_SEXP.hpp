#ifndef QUICKJSR_JSVALUE_TO_SEXP_HPP
#define QUICKJSR_JSVALUE_TO_SEXP_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>

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
    if ((a == String && b == Number) || (a == Number && b == String)) {
      return String;
    }
    if ((a == String && b == Boolean) || (a == Boolean && b == String)) {
      return String;
    }
    return Mixed;
  }
  SEXP JSValue_to_SEXP(JSContext* ctx, const JSValue& val);

  SEXP date_sexp(JSContext* ctx, const JSValue& val) {
    // Extract getIsoString from the Date object
    JSValue iso_str_func = JS_GetPropertyStr(ctx, val, "toISOString");
    if (JS_IsException(iso_str_func) || !JS_IsFunction(ctx, iso_str_func)) {
      JS_FreeValue(ctx, iso_str_func);
      return R_NilValue;
    }
    JSValue iso_str_val = JS_Call(ctx, iso_str_func, val, 0, NULL);
    JS_FreeValue(ctx, iso_str_func);
    if (JS_IsException(iso_str_val) || !JS_IsString(iso_str_val)) {
      JS_FreeValue(ctx, iso_str_val);
      return R_NilValue;
    }
    const char* res = JS_ToCString(ctx, iso_str_val);
    cpp11::function as_posix = cpp11::package("base")["as.POSIXct"];
    SEXP out = as_posix(res, "tz"_nm = "UTC", "format"_nm = "%Y-%m-%dT%H:%M:%OSZ");
    JS_FreeValue(ctx, iso_str_val);
    JS_FreeCString(ctx, res);
    return out;
  }

  SEXP array_sexp(JSContext* ctx, const JSValue& val) {
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
          out[static_cast<R_xlen_t>(i)] = NA_REAL;
        } else {
          double res;
          JS_ToFloat64(ctx, &res, base_val);
          out[static_cast<R_xlen_t>(i)] = res;
        }
        JS_FreeValue(ctx, base_val);
      }
      return out;
    } else if (prev_type == String) {
      cpp11::writable::strings out(len);
      for (int64_t i = 0; i < len; i++) {
        base_val = JS_GetPropertyInt64(ctx, val, i);
        if (JS_IsNull(base_val) || JS_IsUndefined(base_val)) {
          out[static_cast<R_xlen_t>(i)] = NA_STRING;
        } else if (JS_IsBool(base_val)) {
          out[static_cast<R_xlen_t>(i)] = JS_ToBool(ctx, base_val) ? "TRUE" : "FALSE";
        } else if (JS_VALUE_GET_TAG(base_val) == JS_TAG_INT) {
          int32_t res = JS_VALUE_GET_INT(base_val);
          out[static_cast<R_xlen_t>(i)] = std::to_string(res);
        } else if (JS_VALUE_GET_TAG(base_val) == JS_TAG_BIG_INT) {
          int64_t res;
          JS_ToBigInt64(ctx, &res, base_val);
          out[static_cast<R_xlen_t>(i)] = std::to_string(res);
        } else if (JS_VALUE_GET_TAG(base_val) == JS_TAG_SHORT_BIG_INT) {
          int64_t res = JS_VALUE_GET_SHORT_BIG_INT(base_val);
          out[static_cast<R_xlen_t>(i)] = std::to_string(res);
        } else if (JS_TAG_IS_FLOAT64(JS_VALUE_GET_TAG(base_val))) {
          double res;
          JS_ToFloat64(ctx, &res, base_val);
          std::string str_res;
          if (std::isnan(res)) {
            str_res = "NaN";
          } else if (std::isinf(res)) {
            str_res = (res > 0) ? "Inf" : "-Inf";
          } else {
            str_res = std::to_string(res);
            // Remove trailing zeros
            str_res.erase(str_res.find_last_not_of('0') + 1, std::string::npos);
            // If the last character is a decimal point, remove it
            if (str_res.back() == '.') {
              str_res.pop_back();
            }
          }
          out[static_cast<R_xlen_t>(i)] = str_res;
        } else {
          const char* res = JS_ToCString(ctx, base_val);
          out[static_cast<R_xlen_t>(i)] = res;
          JS_FreeCString(ctx, res);
        }
        JS_FreeValue(ctx, base_val);
      }
      return out;
    } else if (prev_type == Boolean || prev_type == Null) {
      cpp11::writable::logicals out(len);
      for (int64_t i = 0; i < len; i++) {
        base_val = JS_GetPropertyInt64(ctx, val, i);
        if (JS_IsNull(base_val) || JS_IsUndefined(base_val)) {
          out[static_cast<R_xlen_t>(i)] = NA_LOGICAL;
        } else {
          out[static_cast<R_xlen_t>(i)] = static_cast<bool>(JS_ToBool(ctx, base_val));
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
        SEXP elem_sexp = JSValue_to_SEXP(ctx, elem);
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

      if (all_double && all_same_size) {
        cpp11::function unlist = cpp11::package("base")["unlist"];
        cpp11::function matrix = cpp11::package("base")["matrix"];
        return matrix(unlist(out), len, first_size, true);
      } else {
        return out;
      }
    }
  }

  SEXP object_sexp(JSContext* ctx, const JSValue& val) {
    // Handle as object
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
    JS_FreePropertyEnum(ctx, tab, len);
    out.attr("names") = keys;
    return out;
  }

SEXP JSValue_to_SEXP(JSContext* ctx, const JSValue& val) {
  switch (JS_VALUE_GET_TAG(val)) {
    case JS_TAG_EXCEPTION: {
      JSValue exc = JS_GetException(ctx);
      const char* res_str = JS_ToCString(ctx, val);
      std::string msg = res_str;
      JS_FreeCString(ctx, res_str);
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
      return cpp11::as_sexp(static_cast<bool>(JS_ToBool(ctx, val)));
    }
    case JS_TAG_INT: {
      return cpp11::as_sexp(JS_VALUE_GET_INT(val));
    }
    case JS_TAG_SHORT_BIG_INT: {
      return cpp11::as_sexp(JS_VALUE_GET_SHORT_BIG_INT(val));
    }
    case JS_TAG_BIG_INT: {
      int64_t res;
      JS_ToBigInt64(ctx, &res, val);
      return cpp11::as_sexp(res);
    }
    case JS_TAG_FLOAT64: {
      if (JS_VALUE_IS_NAN(val)) {
        return cpp11::as_sexp(R_NaN);
      }
      return cpp11::as_sexp(JS_VALUE_GET_FLOAT64(val));
    }
    case JS_TAG_STRING: {
      const char* res = JS_ToCString(ctx, val);
      std::string res_str = res ? res : "";
      JS_FreeCString(ctx, res);
      return cpp11::as_sexp(res_str);
    }
    case JS_TAG_OBJECT: {
      if (JS_IsDate(val)) {
        return date_sexp(ctx, val);
      } else if (JS_IsArray(val)) {
        return array_sexp(ctx, val);
      } else {
        return object_sexp(ctx, val);
      }
    }
    default: {
      return R_NilValue;
    }
  }
  return R_NilValue; // Fallback for unhandled types
}

} // namespace quickjsr

#endif
