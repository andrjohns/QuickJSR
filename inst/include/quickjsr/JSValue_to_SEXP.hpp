#ifndef QUICKJSR_JSVALUE_TO_SEXP_HPP
#define QUICKJSR_JSVALUE_TO_SEXP_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>

namespace quickjsr {

SEXP JSValue_to_SEXP_scalar(JSContext* ctx, JSValue val) {
  if (JS_IsBool(val)) {
    return cpp11::as_sexp(static_cast<bool>(JS_ToBool(ctx, val)));
  }
  if (JS_IsNumber(val)) {
    double res;
    JS_ToFloat64(ctx, &res, val);
    return cpp11::as_sexp(res);
  }
  if (JS_IsString(val)) {
    return cpp11::as_sexp(JS_ToCString(ctx, val));
  }
  return cpp11::as_sexp("Unsupported type");
}

SEXP JSValue_to_SEXP(JSContext* ctx, JSValue val) {
  // TODO: Implement array and object conversion
  return JSValue_to_SEXP_scalar(ctx, val);
}

} // namespace quickjsr

#endif
