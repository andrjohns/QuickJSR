#ifndef QUICKJSR_JSON_TO_JSVALUE_HPP
#define QUICKJSR_JSON_TO_JSVALUE_HPP

#include <quickjsr/wrapper_classes.hpp>

namespace quickjsr {

JSValueWrapper JSON_to_JSValue(JSContext* ctx, const std::string& json) {
  JSValueWrapper global = JS_GetGlobalObject(ctx);
  JSValueWrapper json_obj = JS_GetPropertyStr(ctx, global, "JSON");
  JSValueWrapper parse = JS_GetPropertyStr(ctx, json_obj, "parse");

  JSValueWrapper json_str = JS_NewString(ctx, json.c_str());
  JSValueWrapper result = JS_Call(ctx, parse, global, 1, &json_str);

  if (JS_IsException(result)) {
    js_std_dump_error(ctx);
  }

  return result;
}

} // namespace quickjsr
#endif
