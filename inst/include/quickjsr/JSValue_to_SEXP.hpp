#ifndef QUICKJSR_JSVALUE_TO_SEXP_HPP
#define QUICKJSR_JSVALUE_TO_SEXP_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <quickjsr/JSValue_to_Cpp.hpp>
#include <quickjsr/JSCommonType.hpp>

namespace quickjsr {
// Forward declaration to allow for recursive calls
SEXP JSValue_to_SEXP(JSContext* ctx, JSValue val);

SEXP JSValue_to_SEXP_scalar(JSContext* ctx, JSValue val) {
  if (JS_IsBool(val)) {
    return cpp11::as_sexp(JSValue_to_Cpp<bool>(ctx, val));
  }
  if (JS_IsNumber(val)) {
    return cpp11::as_sexp(JSValue_to_Cpp<double>(ctx, val));
  }
  if (JS_IsString(val)) {
    return cpp11::as_sexp(JSValue_to_Cpp<std::string>(ctx, val));
  }
  return cpp11::as_sexp("Unsupported type");
}

JSCommonType JS_ArrayCommonType(JSContext* ctx, JSValue val) {
  JSValue elem = JS_GetPropertyUint32(ctx, val, 0);
  JSCommonType common_type = JS_GetCommonType(elem);
  JS_FreeValue(ctx, elem);

  uint32_t len;
  JSValue arr_len = JS_GetPropertyStr(ctx, val, "length");
  JS_ToUint32(ctx, &len, arr_len);
  JS_FreeValue(ctx, arr_len);

  for (uint32_t i = 1; i < len; i++) {
    elem = JS_GetPropertyUint32(ctx, val, i);
    common_type = JS_UpdateCommonType(common_type, elem);
    JS_FreeValue(ctx, elem);
  }
  return common_type;
}

SEXP JSValue_to_SEXP_vector(JSContext* ctx, JSValue val) {
  switch (JS_ArrayCommonType(ctx, val)) {
    case Integer:
      return cpp11::as_sexp(JSValue_to_Cpp<std::vector<int>>(ctx, val));
    case Double:
      return cpp11::as_sexp(JSValue_to_Cpp<std::vector<double>>(ctx, val));
    case Logical:
      return cpp11::as_sexp(JSValue_to_Cpp<std::vector<bool>>(ctx, val));
    case Character:
      return cpp11::as_sexp(JSValue_to_Cpp<std::vector<std::string>>(ctx, val));
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
    default:
      return cpp11::as_sexp("Unsupported type");
  }
}

SEXP JSValue_to_SEXP(JSContext* ctx, JSValue val) {
  if (JS_IsArray(ctx, val)) {
    return JSValue_to_SEXP_vector(ctx, val);
  }
  // TODO: Implement array and object conversion
  return JSValue_to_SEXP_scalar(ctx, val);
}

} // namespace quickjsr

#endif
