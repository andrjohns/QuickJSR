#include <cpp11.hpp>
#include <cpp11/declarations.hpp>
#include <R_ext/Rdynload.h>
#include <R_ext/Altrep.h>
#include <quickjsr/JSValueRef.hpp>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>

namespace {
  struct JSBufferALTREPData {
    const uint8_t* data;
    R_xlen_t length;
    SEXPTYPE type;
  };

  R_altrep_class_t raw_class;
  R_altrep_class_t integer_class;
  R_altrep_class_t real_class;

  JSBufferALTREPData* altrep_data(SEXP x) {
    return static_cast<JSBufferALTREPData*>(
      R_ExternalPtrAddr(R_altrep_data1(x))
    );
  }

  SEXP materialized(SEXP x) {
    return R_altrep_data2(x);
  }

  void* writable_data(SEXP x, SEXPTYPE type) {
#if R_VERSION >= R_Version(4, 6, 0)
    return DATAPTR_RW(x);
#else
    if (type == RAWSXP) return RAW(x);
    if (type == INTSXP) return INTEGER(x);
    return REAL(x);
#endif
  }

  SEXP materialize(SEXP x) {
    SEXP existing = materialized(x);
    if (existing != R_NilValue) return existing;

    JSBufferALTREPData* data = altrep_data(x);
    SEXP result = PROTECT(Rf_allocVector(data->type, data->length));
    size_t bytes = 0;
    if (data->type == RAWSXP) {
      bytes = static_cast<size_t>(data->length);
    } else if (data->type == INTSXP) {
      bytes = static_cast<size_t>(data->length) * sizeof(int);
    } else {
      bytes = static_cast<size_t>(data->length) * sizeof(double);
    }
    if (bytes > 0) {
      std::memcpy(writable_data(result, data->type), data->data, bytes);
    }
    R_set_altrep_data2(x, result);
    data->data = nullptr;
    R_SetExternalPtrProtected(R_altrep_data1(x), R_NilValue);
    UNPROTECT(1);
    return result;
  }

  void finalize_data(SEXP holder) {
    delete static_cast<JSBufferALTREPData*>(R_ExternalPtrAddr(holder));
    R_ClearExternalPtr(holder);
  }

  R_xlen_t view_length(SEXP x) {
    return altrep_data(x)->length;
  }

  void* view_dataptr(SEXP x, Rboolean writeable) {
    if (writeable || materialized(x) != R_NilValue) {
      return writable_data(materialize(x), altrep_data(x)->type);
    }
    return const_cast<uint8_t*>(altrep_data(x)->data);
  }

  const void* view_dataptr_or_null(SEXP x) {
    SEXP value = materialized(x);
    if (value != R_NilValue) return DATAPTR_RO(value);
    return altrep_data(x)->data;
  }

  Rbyte raw_elt(SEXP x, R_xlen_t i) {
    SEXP value = materialized(x);
    if (value != R_NilValue) return RAW_ELT(value, i);
    return reinterpret_cast<const Rbyte*>(altrep_data(x)->data)[i];
  }

  R_xlen_t raw_region(SEXP x, R_xlen_t i, R_xlen_t n, Rbyte* output) {
    R_xlen_t count = std::min(n, view_length(x) - i);
    if (count <= 0) return 0;
    SEXP value = materialized(x);
    const Rbyte* source = value != R_NilValue
      ? RAW(value) + i
      : reinterpret_cast<const Rbyte*>(altrep_data(x)->data) + i;
    std::copy_n(source, count, output);
    return count;
  }

  int integer_elt(SEXP x, R_xlen_t i) {
    SEXP value = materialized(x);
    if (value != R_NilValue) return INTEGER_ELT(value, i);
    return reinterpret_cast<const int*>(altrep_data(x)->data)[i];
  }

  R_xlen_t integer_region(SEXP x, R_xlen_t i, R_xlen_t n, int* output) {
    R_xlen_t count = std::min(n, view_length(x) - i);
    if (count <= 0) return 0;
    SEXP value = materialized(x);
    const int* source = value != R_NilValue
      ? INTEGER(value) + i
      : reinterpret_cast<const int*>(altrep_data(x)->data) + i;
    std::copy_n(source, count, output);
    return count;
  }

  double real_elt(SEXP x, R_xlen_t i) {
    SEXP value = materialized(x);
    if (value != R_NilValue) return REAL_ELT(value, i);
    return reinterpret_cast<const double*>(altrep_data(x)->data)[i];
  }

  R_xlen_t real_region(SEXP x, R_xlen_t i, R_xlen_t n, double* output) {
    R_xlen_t count = std::min(n, view_length(x) - i);
    if (count <= 0) return 0;
    SEXP value = materialized(x);
    const double* source = value != R_NilValue
      ? REAL(value) + i
      : reinterpret_cast<const double*>(altrep_data(x)->data) + i;
    std::copy_n(source, count, output);
    return count;
  }
}

extern "C" void quickjsr_init_altrep(DllInfo* dll) {
  raw_class = R_make_altraw_class("js_buffer_raw", "QuickJSR", dll);
  integer_class = R_make_altinteger_class(
    "js_buffer_integer", "QuickJSR", dll
  );
  real_class = R_make_altreal_class("js_buffer_real", "QuickJSR", dll);

  R_set_altrep_Length_method(raw_class, view_length);
  R_set_altvec_Dataptr_method(raw_class, view_dataptr);
  R_set_altvec_Dataptr_or_null_method(raw_class, view_dataptr_or_null);
  R_set_altraw_Elt_method(raw_class, raw_elt);
  R_set_altraw_Get_region_method(raw_class, raw_region);

  R_set_altrep_Length_method(integer_class, view_length);
  R_set_altvec_Dataptr_method(integer_class, view_dataptr);
  R_set_altvec_Dataptr_or_null_method(integer_class, view_dataptr_or_null);
  R_set_altinteger_Elt_method(integer_class, integer_elt);
  R_set_altinteger_Get_region_method(integer_class, integer_region);

  R_set_altrep_Length_method(real_class, view_length);
  R_set_altvec_Dataptr_method(real_class, view_dataptr);
  R_set_altvec_Dataptr_or_null_method(real_class, view_dataptr_or_null);
  R_set_altreal_Elt_method(real_class, real_elt);
  R_set_altreal_Get_region_method(real_class, real_region);
}

extern "C" SEXP qjs_value_ref_to_altrep_(SEXP ref_ptr_) {
  BEGIN_CPP11
  if (Rf_inherits(ref_ptr_, "JSCompiledScript")) {
    cpp11::stop("compiled scripts cannot be converted to ALTREP views");
  }
  quickjsr::JSValueRefData* ref = quickjsr::get_value_ref(ref_ptr_);
  int js_type = JS_GetTypedArrayType(ref->value);
  SEXPTYPE r_type;
  if (js_type == JS_TYPED_ARRAY_UINT8 || js_type == JS_TYPED_ARRAY_UINT8C) {
    r_type = RAWSXP;
  } else if (js_type == JS_TYPED_ARRAY_INT32) {
    r_type = INTSXP;
  } else if (js_type == JS_TYPED_ARRAY_FLOAT64) {
    r_type = REALSXP;
  } else {
    cpp11::stop(
      "ALTREP views require a Uint8Array, Uint8ClampedArray, Int32Array, or Float64Array"
    );
  }

  size_t offset = 0;
  size_t byte_length = 0;
  size_t element_size = 0;
  JSValue buffer = JS_GetTypedArrayBuffer(
    ref->ctx, ref->value, &offset, &byte_length, &element_size
  );
  if (JS_IsException(buffer)) {
    JSValue exception = JS_GetException(ref->ctx);
    JS_FreeValue(ref->ctx, exception);
    cpp11::stop("typed array buffer is detached or out of bounds");
  }
  if (JS_SetImmutableArrayBuffer(buffer, true) < 0) {
    JS_FreeValue(ref->ctx, buffer);
    cpp11::stop("ALTREP views require a regular ArrayBuffer");
  }

  size_t buffer_size = 0;
  uint8_t* buffer_data = JS_GetArrayBuffer(ref->ctx, &buffer_size, buffer);
  if ((!buffer_data && byte_length > 0) || offset > buffer_size ||
      byte_length > buffer_size - offset) {
    JSValue exception = JS_GetException(ref->ctx);
    JS_FreeValue(ref->ctx, exception);
    JS_FreeValue(ref->ctx, buffer);
    cpp11::stop("typed array buffer is unavailable");
  }
  const uint8_t* data_ptr = buffer_data ? buffer_data + offset : nullptr;
  JS_FreeValue(ref->ctx, buffer);

  auto data = std::make_unique<JSBufferALTREPData>(JSBufferALTREPData{
    data_ptr,
    static_cast<R_xlen_t>(byte_length / element_size),
    r_type
  });
  SEXP holder = PROTECT(R_MakeExternalPtr(nullptr, R_NilValue, ref_ptr_));
  R_SetExternalPtrAddr(holder, data.release());
  R_RegisterCFinalizerEx(holder, finalize_data, TRUE);

  R_altrep_class_t altrep_class = r_type == RAWSXP
    ? raw_class
    : r_type == INTSXP ? integer_class : real_class;
  SEXP result = R_new_altrep(altrep_class, holder, R_NilValue);
  UNPROTECT(1);
  return result;
  END_CPP11
}
