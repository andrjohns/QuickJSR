#include <quickjs.h>
#include <stdbool.h>
#include <string.h>

bool qjs_source_impl(JSContext* ctx, const char* code_string) {
  JSValue val = JS_Eval(ctx, code_string, strlen(code_string), "", 0);
  bool succeeded = !JS_IsException(val);
  JS_FreeValue(ctx, val);

  return succeeded;
}

bool qjs_validate_impl(JSContext* ctx, const char* function_name) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue val = JS_GetPropertyStr(ctx, global, function_name);

  bool succeeded = !JS_IsException(val);
  JS_FreeValue(ctx, val);
  JS_FreeValue(ctx, global);

  return succeeded;
}

const char* JS_ValToJSON_impl(JSContext* ctx, JSValue* val) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue json = JS_GetPropertyStr(ctx, global, "JSON");
  JSValue stringify = JS_GetPropertyStr(ctx, json, "stringify");

  JSValue result_js = JS_Call(ctx, stringify, global, 1, val);
  const char* json_string = JS_ToCString(ctx, result_js);

  JS_FreeValue(ctx, result_js);
  JS_FreeValue(ctx, stringify);
  JS_FreeValue(ctx, json);
  JS_FreeValue(ctx, global);

  return json_string;
}

const char* qjs_call_impl(JSContext* ctx, const char* wrapped_name,
                      const char* call_wrapper, const char* args_json) {

  JSValue tmp = JS_Eval(ctx, call_wrapper, strlen(call_wrapper), "", 0);
  JS_FreeValue(ctx, tmp);

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue function_wrapper = JS_GetPropertyStr(ctx, global, wrapped_name);
  JSValue args[] = {
    JS_NewString(ctx, args_json)
  };

  JSValue result_js = JS_Call(ctx, function_wrapper, global, 1, args);
  const char* result = JS_ValToJSON_impl(ctx, &result_js);

  JS_FreeValue(ctx, result_js);
  JS_FreeValue(ctx, args[0]);
  JS_FreeValue(ctx, function_wrapper);
  JS_FreeValue(ctx, global);

  return result;
}

const char* qjs_eval_impl(const char* eval_string) {
  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  JSValue val = JS_Eval(ctx, eval_string, strlen(eval_string), "", 0);
  const char* result = JS_ValToJSON_impl(ctx, &val);

  JS_FreeValue(ctx, val);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  return result;
}
