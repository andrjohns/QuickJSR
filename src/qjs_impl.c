#include <quickjs-libc.h>
#include <stdbool.h>
#include <string.h>

bool qjs_source_impl(JSContext* ctx, const char* code_string) {
  JSValue val = JS_Eval(ctx, code_string, strlen(code_string), "", 0);
  bool failed = JS_IsException(val);
  if (failed) {
    js_std_dump_error(ctx);
  }
  JS_FreeValue(ctx, val);

  return !failed;
}

bool qjs_validate_impl(JSContext* ctx, const char* function_name) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue val = JS_GetPropertyStr(ctx, global, function_name);

  bool failed = JS_IsException(val);
  if (failed) {
    js_std_dump_error(ctx);
  }
  JS_FreeValue(ctx, val);
  JS_FreeValue(ctx, global);

  return !failed;
}

const char* JS_ValToJSON(JSContext* ctx, JSValue* val) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue json = JS_GetPropertyStr(ctx, global, "JSON");
  JSValue stringify = JS_GetPropertyStr(ctx, json, "stringify");

  JSValue result_js = JS_Call(ctx, stringify, global, 1, val);
  const char* result;
  if (JS_IsException(result_js)) {
    js_std_dump_error(ctx);
    result = "Error!";
  } else {
    result = JS_ToCString(ctx, result_js);
  }

  JS_FreeValue(ctx, result_js);
  JS_FreeValue(ctx, stringify);
  JS_FreeValue(ctx, json);
  JS_FreeValue(ctx, global);

  return result;
}

const char* qjs_call_impl(JSContext* ctx, const char* wrapped_name,
                          const char* call_wrapper, const char* args_json) {
  JSValue tmp = JS_Eval(ctx, call_wrapper, strlen(call_wrapper), "", 0);
  bool failed = JS_IsException(tmp);
  JS_FreeValue(ctx, tmp);
  if (failed) {
    js_std_dump_error(ctx);
    return "Error!";
  }

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue function_wrapper = JS_GetPropertyStr(ctx, global, wrapped_name);
  JSValue args[] = {
    JS_NewString(ctx, args_json)
  };

  JSValue result_js = JS_Call(ctx, function_wrapper, global, 1, args);
  const char* result;
  if (JS_IsException(result_js)) {
    js_std_dump_error(ctx);
    result = "Error!";
  } else {
    result = JS_ValToJSON(ctx, &result_js);
  }

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
  const char* result;
  if (JS_IsException(val)) {
    js_std_dump_error(ctx);
    result = "Error!";
  } else {
    result = JS_ValToJSON(ctx, &val);
  }

  JS_FreeValue(ctx, val);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  return result;
}
