#ifndef QUICKJSR_JSON_TO_JSVALUE_HPP
#define QUICKJSR_JSON_TO_JSVALUE_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>

namespace quickjsr {

JSValue JSON_to_JSValue(JSContext* ctx, const std::string& json) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue json_obj = JS_GetPropertyStr(ctx, global, "JSON");
  JSValue parse = JS_GetPropertyStr(ctx, json_obj, "parse");

  JSValue json_str = JS_NewString(ctx, json.c_str());
  JSValue result = JS_Call(ctx, parse, global, 1, &json_str);

  if (JS_IsException(result)) {
    js_std_dump_error(ctx);
  }

  JS_FreeValue(ctx, json_str);
  JS_FreeValue(ctx, parse);
  JS_FreeValue(ctx, json_obj);
  JS_FreeValue(ctx, global);

  return result;
}

} // namespace quickjsr
#endif
