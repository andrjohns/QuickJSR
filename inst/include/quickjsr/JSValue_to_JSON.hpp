#ifndef QUICKJSR_JSVALUE_TO_JSON_HPP
#define QUICKJSR_JSVALUE_TO_JSON_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>

namespace quickjsr {

std::string JS_ValToJSON(JSContext* ctx, JSValue* val) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue json = JS_GetPropertyStr(ctx, global, "JSON");
  JSValue stringify = JS_GetPropertyStr(ctx, json, "stringify");

  JSValue result_js = JS_Call(ctx, stringify, global, 1, val);
  std::string result;
  if (JS_IsException(result_js)) {
    js_std_dump_error(ctx);
    result = "Error!";
  } else {
    result = JSValue_to_Cpp<std::string>(ctx, result_js);
  }

  JS_FreeValue(ctx, result_js);
  JS_FreeValue(ctx, stringify);
  JS_FreeValue(ctx, json);
  JS_FreeValue(ctx, global);

  return result;
}

} // namespace quickjsr

#endif
