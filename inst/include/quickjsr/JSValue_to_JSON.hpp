#ifndef QUICKJSR_JSVALUE_TO_JSON_HPP
#define QUICKJSR_JSVALUE_TO_JSON_HPP

#include <quickjsr/wrapper_classes.hpp>

namespace quickjsr {

std::string JSValue_to_JSON(JSContext* ctx, JSValue* val) {
  JSValueWrapper global = JS_GetGlobalObject(ctx);
  JSValueWrapper json = JS_GetPropertyStr(ctx, global, "JSON");
  JSValueWrapper stringify = JS_GetPropertyStr(ctx, json, "stringify");

  JSValueWrapper result_js = JS_Call(ctx, stringify, global, 1, val);
  std::string result;
  if (JS_IsException(result_js)) {
    js_std_dump_error(ctx);
    result = "Error!";
  } else {
    result = JSValue_to_Cpp<std::string>(ctx, result_js);
  }

  return result;
}

} // namespace quickjsr

#endif
