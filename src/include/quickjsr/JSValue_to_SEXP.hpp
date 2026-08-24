#ifndef QUICKJSR_JSVALUE_TO_SEXP_HPP
#define QUICKJSR_JSVALUE_TO_SEXP_HPP

#include "cpp11/protect.hpp"
#include "quickjs.h"
#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <quickjsr/MaskedTypedArray.hpp>
#include <quickjsr/RVectorView.hpp>
#include <algorithm>
#include <climits>
#include <cstdint>

extern "C" int quickjsr_get_date_epoch_ms(JSContext* ctx, JSValueConst value,
                                           double* result);
extern "C" int quickjsr_get_fast_array_data(JSContext* ctx, JSValueConst value,
                                             const JSValue** result,
                                             uint32_t* size);

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

  inline BaseType value_to_base_type(const JSValue& value) {
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

  inline BaseType combine_array_types(BaseType a, BaseType b) {
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

  inline SEXP typed_array_sexp(JSContext* ctx, const JSValue& val, int type) {
    size_t offset = 0;
    size_t byte_length = 0;
    size_t element_size = 0;
    JSValue buffer = JS_GetTypedArrayBuffer(
      ctx, val, &offset, &byte_length, &element_size
    );
    if (JS_IsException(buffer)) {
      return JSValue_to_SEXP(ctx, buffer);
    }
    size_t buffer_size = 0;
    uint8_t* buffer_data = JS_GetArrayBuffer(ctx, &buffer_size, buffer);
    JS_FreeValue(ctx, buffer);
    if (!buffer_data && byte_length > 0) {
      return JSValue_to_SEXP(ctx, JS_EXCEPTION);
    }
    const uint8_t* data = buffer_data ? buffer_data + offset : nullptr;
    R_xlen_t size = static_cast<R_xlen_t>(byte_length / element_size);

    if (type == JS_TYPED_ARRAY_UINT8 || type == JS_TYPED_ARRAY_UINT8C) {
      cpp11::writable::raws out(size);
      if (size > 0) std::copy_n(data, size, RAW(out));
      return out;
    }
    if (type == JS_TYPED_ARRAY_INT32) {
      cpp11::writable::integers out(size);
      const int32_t* values = reinterpret_cast<const int32_t*>(data);
      if (size > 0) std::copy_n(values, size, INTEGER(out));
      return out;
    }
    if (type == JS_TYPED_ARRAY_FLOAT64) {
      cpp11::writable::doubles out(size);
      const double* values = reinterpret_cast<const double*>(data);
      if (size > 0) std::copy_n(values, size, REAL(out));
      return out;
    }
    if (type == JS_TYPED_ARRAY_INT8) {
      cpp11::writable::integers out(size);
      const int8_t* values = reinterpret_cast<const int8_t*>(data);
      if (size > 0) std::copy_n(values, size, INTEGER(out));
      return out;
    }
    if (type == JS_TYPED_ARRAY_INT16) {
      cpp11::writable::integers out(size);
      const int16_t* values = reinterpret_cast<const int16_t*>(data);
      if (size > 0) std::copy_n(values, size, INTEGER(out));
      return out;
    }
    if (type == JS_TYPED_ARRAY_UINT16) {
      cpp11::writable::integers out(size);
      const uint16_t* values = reinterpret_cast<const uint16_t*>(data);
      if (size > 0) std::copy_n(values, size, INTEGER(out));
      return out;
    }
    if (type == JS_TYPED_ARRAY_UINT32) {
      cpp11::writable::doubles out(size);
      const uint32_t* values = reinterpret_cast<const uint32_t*>(data);
      if (size > 0) std::copy_n(values, size, REAL(out));
      return out;
    }
    if (type == JS_TYPED_ARRAY_FLOAT32) {
      cpp11::writable::doubles out(size);
      const float* values = reinterpret_cast<const float*>(data);
      if (size > 0) std::copy_n(values, size, REAL(out));
      return out;
    }
    if (type == JS_TYPED_ARRAY_BIG_INT64) {
      cpp11::writable::doubles out(size);
      const int64_t* values = reinterpret_cast<const int64_t*>(data);
      if (size > 0) std::copy_n(values, size, REAL(out));
      return out;
    }
    if (type == JS_TYPED_ARRAY_BIG_UINT64) {
      cpp11::writable::doubles out(size);
      const uint64_t* values = reinterpret_cast<const uint64_t*>(data);
      if (size > 0) std::copy_n(values, size, REAL(out));
      return out;
    }

    cpp11::writable::doubles out(size);
    for (R_xlen_t i = 0; i < size; i++) {
      JSValue element = JS_GetPropertyInt64(ctx, val, i);
      double value;
      if (JS_ToFloat64(ctx, &value, element)) {
        JS_FreeValue(ctx, element);
        return JSValue_to_SEXP(ctx, JS_EXCEPTION);
      }
      JS_FreeValue(ctx, element);
      out[i] = value;
    }
    return out;
  }

  inline SEXP date_sexp(JSContext* ctx, const JSValue& val) {
    double epoch_ms;
    if (quickjsr_get_date_epoch_ms(ctx, val, &epoch_ms)) {
      return R_NilValue;
    }

    SEXP out = PROTECT(Rf_allocVector(REALSXP, 1));
    REAL(out)[0] = epoch_ms / 1000.0;
    Rf_setAttrib(out, R_ClassSymbol, Rf_mkString("POSIXct"));
    UNPROTECT(1);
    return out;
  }

  inline SEXP masked_typed_array_sexp(
    JSContext* ctx, const JSValue& val, MaskedTypedArrayData* metadata
  ) {
    JSValue values = JS_GetPropertyStr(ctx, val, "values");
    if (JS_IsException(values)) return JSValue_to_SEXP(ctx, values);
    JSValue validity = JS_GetPropertyStr(ctx, val, "validity");
    if (JS_IsException(validity)) {
      JS_FreeValue(ctx, values);
      return JSValue_to_SEXP(ctx, validity);
    }

    int expected_type = metadata->type == LGLSXP
      ? JS_TYPED_ARRAY_UINT8
      : metadata->type == INTSXP
        ? JS_TYPED_ARRAY_INT32
        : JS_TYPED_ARRAY_FLOAT64;
    if (JS_GetTypedArrayType(values) != expected_type ||
        JS_GetTypedArrayType(validity) != JS_TYPED_ARRAY_UINT8) {
      JS_FreeValue(ctx, values);
      JS_FreeValue(ctx, validity);
      cpp11::stop("masked typed array buffers have invalid types");
    }

    size_t value_offset = 0;
    size_t value_bytes = 0;
    size_t value_element_size = 0;
    JSValue value_buffer = JS_GetTypedArrayBuffer(
      ctx, values, &value_offset, &value_bytes, &value_element_size
    );
    if (JS_IsException(value_buffer)) {
      JS_FreeValue(ctx, values);
      JS_FreeValue(ctx, validity);
      return JSValue_to_SEXP(ctx, value_buffer);
    }

    size_t validity_offset = 0;
    size_t validity_bytes = 0;
    size_t validity_element_size = 0;
    JSValue validity_buffer = JS_GetTypedArrayBuffer(
      ctx, validity, &validity_offset, &validity_bytes,
      &validity_element_size
    );
    if (JS_IsException(validity_buffer)) {
      JS_FreeValue(ctx, value_buffer);
      JS_FreeValue(ctx, values);
      JS_FreeValue(ctx, validity);
      return JSValue_to_SEXP(ctx, validity_buffer);
    }

    size_t value_buffer_size = 0;
    size_t validity_buffer_size = 0;
    uint8_t* value_buffer_data = JS_GetArrayBuffer(
      ctx, &value_buffer_size, value_buffer
    );
    uint8_t* validity_buffer_data = JS_GetArrayBuffer(
      ctx, &validity_buffer_size, validity_buffer
    );
    size_t size = value_bytes / value_element_size;
    bool invalid = (!value_buffer_data && value_bytes > 0) ||
      (!validity_buffer_data && validity_bytes > 0) ||
      value_offset > value_buffer_size ||
      value_bytes > value_buffer_size - value_offset ||
      validity_offset > validity_buffer_size ||
      validity_bytes > validity_buffer_size - validity_offset ||
      validity_element_size != 1 || validity_bytes != size;
    if (invalid) {
      JSValue exception = JS_GetException(ctx);
      JS_FreeValue(ctx, exception);
      JS_FreeValue(ctx, value_buffer);
      JS_FreeValue(ctx, validity_buffer);
      JS_FreeValue(ctx, values);
      JS_FreeValue(ctx, validity);
      cpp11::stop("masked typed array buffers are unavailable or have different lengths");
    }

    const uint8_t* value_data = value_buffer_data
      ? value_buffer_data + value_offset
      : nullptr;
    const uint8_t* mask = validity_buffer_data
      ? validity_buffer_data + validity_offset
      : nullptr;
    SEXP out = PROTECT(Rf_allocVector(metadata->type, size));
    if (metadata->type == LGLSXP) {
      for (size_t i = 0; i < size; i++) {
        LOGICAL(out)[i] = mask[i] ? value_data[i] != 0 : NA_LOGICAL;
      }
    } else if (metadata->type == INTSXP) {
      const int32_t* source = reinterpret_cast<const int32_t*>(value_data);
      if (size > 0) std::copy_n(source, size, INTEGER(out));
      for (size_t i = 0; i < size; i++) {
        if (!mask[i]) INTEGER(out)[i] = NA_INTEGER;
      }
    } else {
      const double* source = reinterpret_cast<const double*>(value_data);
      if (size > 0) std::copy_n(source, size, REAL(out));
      for (size_t i = 0; i < size; i++) {
        if (!mask[i]) REAL(out)[i] = NA_REAL;
      }
    }
    JS_FreeValue(ctx, value_buffer);
    JS_FreeValue(ctx, validity_buffer);
    JS_FreeValue(ctx, values);
    JS_FreeValue(ctx, validity);
    UNPROTECT(1);
    return out;
  }

  inline SEXP array_sexp(JSContext* ctx, const JSValue& val) {
    int64_t len;
    const JSValue* fast_values = nullptr;
    uint32_t fast_size = 0;
    bool is_fast = quickjsr_get_fast_array_data(ctx, val, &fast_values, &fast_size);
    if (is_fast) {
      len = fast_size;
    } else if (JS_GetLength(ctx, val, &len)) {
      return JSValue_to_SEXP(ctx, JS_EXCEPTION);
    }

    JSValue base_val = JS_UNDEFINED;
    BaseType prev_type = Null;
    if (len > 0) {
      if (is_fast) {
        prev_type = value_to_base_type(fast_values[0]);
      } else {
        base_val = JS_GetPropertyInt64(ctx, val, 0);
        prev_type = value_to_base_type(base_val);
        JS_FreeValue(ctx, base_val);
      }
    }

    for (int64_t i = 1; i < len; i++) {
      BaseType curr_type;
      if (is_fast) {
        curr_type = value_to_base_type(fast_values[i]);
      } else {
        base_val = JS_GetPropertyInt64(ctx, val, i);
        curr_type = value_to_base_type(base_val);
        JS_FreeValue(ctx, base_val);
      }
      prev_type = combine_array_types(prev_type, curr_type);
      if (prev_type == Mixed) {
        // No need to continue checking types, we know it's mixed
        break;
      }
    }

    if (prev_type == Number) {
      cpp11::writable::doubles out(len);
      for (int64_t i = 0; i < len; i++) {
        base_val = is_fast ? fast_values[i] : JS_GetPropertyInt64(ctx, val, i);
        if (JS_IsNull(base_val) || JS_IsUndefined(base_val)) {
          out[static_cast<R_xlen_t>(i)] = NA_REAL;
        } else {
          double res;
          JS_ToFloat64(ctx, &res, base_val);
          out[static_cast<R_xlen_t>(i)] = res;
        }
        if (!is_fast) JS_FreeValue(ctx, base_val);
      }
      return out;
    } else if (prev_type == String) {
      cpp11::writable::strings out(len);
      for (int64_t i = 0; i < len; i++) {
        base_val = is_fast ? fast_values[i] : JS_GetPropertyInt64(ctx, val, i);
        if (JS_IsNull(base_val) || JS_IsUndefined(base_val)) {
          out[static_cast<R_xlen_t>(i)] = NA_STRING;
        } else if (JS_IsBool(base_val)) {
          out[static_cast<R_xlen_t>(i)] = JS_ToBool(ctx, base_val) ? "TRUE" : "FALSE";
        } else if (JS_VALUE_GET_NORM_TAG(base_val) == JS_TAG_INT) {
          int32_t res = JS_VALUE_GET_INT(base_val);
          out[static_cast<R_xlen_t>(i)] = std::to_string(res);
        } else if (JS_VALUE_GET_NORM_TAG(base_val) == JS_TAG_BIG_INT) {
          int64_t res;
          JS_ToBigInt64(ctx, &res, base_val);
          out[static_cast<R_xlen_t>(i)] = std::to_string(res);
        } else if (JS_VALUE_GET_NORM_TAG(base_val) == JS_TAG_SHORT_BIG_INT) {
          int64_t res = JS_VALUE_GET_SHORT_BIG_INT(base_val);
          out[static_cast<R_xlen_t>(i)] = std::to_string(res);
        } else if (JS_TAG_IS_FLOAT64(JS_VALUE_GET_NORM_TAG(base_val))) {
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
        if (!is_fast) JS_FreeValue(ctx, base_val);
      }
      return out;
    } else if (prev_type == Boolean || prev_type == Null) {
      cpp11::writable::logicals out(len);
      for (int64_t i = 0; i < len; i++) {
        base_val = is_fast ? fast_values[i] : JS_GetPropertyInt64(ctx, val, i);
        if (JS_IsNull(base_val) || JS_IsUndefined(base_val)) {
          out[static_cast<R_xlen_t>(i)] = NA_LOGICAL;
        } else {
          out[static_cast<R_xlen_t>(i)] = static_cast<bool>(JS_ToBool(ctx, base_val));
        }
        if (!is_fast) JS_FreeValue(ctx, base_val);
      }
      return out;
    } else {
      // Mixed types, return as list
      cpp11::writable::list out(len);
      bool all_double = true;
      bool all_same_size = true;
      int64_t first_size = -1;
      for (int64_t i = 0; i < len; i++) {
        JSValue elem = is_fast ? fast_values[i] : JS_GetPropertyInt64(ctx, val, i);
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
        if (!is_fast) JS_FreeValue(ctx, elem);
      }

      if (all_double && all_same_size && len <= INT_MAX && first_size <= INT_MAX) {
        SEXP matrix = PROTECT(Rf_allocMatrix(
          REALSXP, static_cast<int>(len), static_cast<int>(first_size)
        ));
        double* target = REAL(matrix);
        for (int64_t i = 0; i < len; i++) {
          SEXP row = out[static_cast<R_xlen_t>(i)];
          const double* source = REAL(row);
          for (int64_t j = 0; j < first_size; j++) {
            target[i + j * len] = source[j];
          }
        }
        UNPROTECT(1);
        return matrix;
      } else {
        return out;
      }
    }
  }

  inline SEXP object_sexp(JSContext* ctx, const JSValue& val) {
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

inline SEXP JSValue_to_SEXP(JSContext* ctx, const JSValue& val) {
  switch (JS_VALUE_GET_NORM_TAG(val)) {
    case JS_TAG_EXCEPTION: {
      JSValue exc = JS_GetException(ctx);
      const char* res_str = JS_ToCString(ctx, exc);
      std::string msg = res_str;
      JS_FreeCString(ctx, res_str);
      std::string stack = "";
      if (JS_IsError(exc)) {
        JSValue stack_val = JS_GetPropertyStr(ctx, exc, "stack");
        const char* stack_str = JS_ToCString(ctx, stack_val);
        stack = stack_str;
        stack = "\n" + stack;
        JS_FreeCString(ctx, stack_str);
        JS_FreeValue(ctx, stack_val);
      }
      JS_FreeValue(ctx, exc);
      cpp11::stop("JavaScript Exception: \n" + msg + stack);
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
      SEXP out = PROTECT(Rf_allocVector(STRSXP, 1));
      SET_STRING_ELT(out, 0, res ? Rf_mkCharLenCE(res, static_cast<int>(strlen(res)), CE_UTF8) : Rf_mkChar(""));
      JS_FreeCString(ctx, res);
      UNPROTECT(1);
      return out;
    }
    case JS_TAG_STRING_ROPE: {
      const char* res = JS_ToCString(ctx, val);
      SEXP out = PROTECT(Rf_allocVector(STRSXP, 1));
      SET_STRING_ELT(out, 0, res ? Rf_mkCharLenCE(res, static_cast<int>(strlen(res)), CE_UTF8) : Rf_mkChar(""));
      JS_FreeCString(ctx, res);
      UNPROTECT(1);
      return out;
    }
    case JS_TAG_OBJECT: {
      if (JS_GetClassID(val) == js_masked_typed_array_class_id) {
        auto* metadata = static_cast<MaskedTypedArrayData*>(
          JS_GetOpaque(val, js_masked_typed_array_class_id)
        );
        return masked_typed_array_sexp(ctx, val, metadata);
      }
      auto* view = static_cast<RVectorViewData*>(
        JS_GetOpaque(val, js_rvector_view_class_id)
      );
      if (view) return view->source;
      int typed_array_type = JS_GetTypedArrayType(val);
      if (typed_array_type >= 0) {
        return typed_array_sexp(ctx, val, typed_array_type);
      } else if (JS_IsDate(val)) {
        return date_sexp(ctx, val);
      } else if (JS_IsArray(val)) {
        return array_sexp(ctx, val);
      } else {
        return object_sexp(ctx, val);
      }
    }
    default: {
      cpp11::stop("Unknown JS TAG: %d", JS_VALUE_GET_NORM_TAG(val));
      return R_NilValue;
    }
  }
  return R_NilValue; // Fallback for unhandled types
}

} // namespace quickjsr

#endif
