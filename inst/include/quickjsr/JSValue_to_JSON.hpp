#ifndef QUICKJSR_JSVALUE_TO_JSON_HPP
#define QUICKJSR_JSVALUE_TO_JSON_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>

namespace quickjsr {

std::string JSValue_to_JSON(JSContext* ctx, JSValue val) {

  JSValue result_js = JS_JSONStringify(ctx, val, JS_UNDEFINED, JS_UNDEFINED);
  std::string result;
  if (JS_IsException(result_js)) {
    js_std_dump_error(ctx);
    result = "Error!";
  } else {
    result = JSValue_to_Cpp<std::string>(ctx, result_js);
  }

  JS_FreeValue(ctx, result_js);

  return result;
}

} // namespace quickjsr

#endif
