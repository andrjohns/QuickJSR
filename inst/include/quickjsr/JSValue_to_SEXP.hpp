#ifndef QUICKJSR_JSVALUE_TO_SEXP_HPP
#define QUICKJSR_JSVALUE_TO_SEXP_HPP

#include <quickjsr/wrapper_classes.hpp>
#include <quickjsr/JSValue_to_Cpp.hpp>

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

SEXP JSValue_to_SEXP_vector(JSContext* ctx, JSValue val) {
  JSValueWrapper elem = JS_GetPropertyUint32(ctx, val, 0);
  SEXP rtn;
  if (JS_IsBool(elem)) {
    rtn = cpp11::as_sexp(JSValue_to_Cpp<std::vector<bool>>(ctx, val));
  } else if (JS_IsNumber(elem)) {
    rtn = cpp11::as_sexp(JSValue_to_Cpp<std::vector<double>>(ctx, val));
  } else if (JS_IsString(elem)) {
    rtn = cpp11::as_sexp(JSValue_to_Cpp<std::vector<std::string>>(ctx, val));
  } else {
    rtn = cpp11::as_sexp("Unsupported type");
  }
  return rtn;
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
