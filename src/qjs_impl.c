#include <quickjs.h>
#include <stdbool.h>
#include <string.h>

bool qjs_source_impl(JSContext* ctx, const char* code_string) {
  JSValue val = JS_Eval(ctx, code_string, strlen(code_string), "", 0);
  bool succeeded = !(JS_IsException(val) || JS_IsError(ctx, val));
  JS_FreeValue(ctx, val);

  return succeeded;
}

bool qjs_validate_impl(JSContext* ctx, const char* function_name) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue val = JS_GetPropertyStr(ctx, global, function_name);

  bool succeeded = !(JS_IsException(val) || JS_IsError(ctx, val));
  JS_FreeValue(ctx, val);
  JS_FreeValue(ctx, global);

  return succeeded;
}

const char* JS_ValToJSON(JSContext* ctx, JSValue* val) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue json = JS_GetPropertyStr(ctx, global, "JSON");
  JSValue stringify = JS_GetPropertyStr(ctx, json, "stringify");

  JSValue result_js = JS_Call(ctx, stringify, global, 1, val);
  const char* result;
  if (JS_IsException(result_js) || JS_IsError(ctx, result_js)) {
    result = "Error in JSON.stringify()!";
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
  bool failed = (JS_IsException(tmp) || JS_IsError(ctx, tmp));
  JS_FreeValue(ctx, tmp);
  if (failed) {
    return "Error initialising function!";
  }

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue function_wrapper = JS_GetPropertyStr(ctx, global, wrapped_name);
  JSValue args[] = {
    JS_NewString(ctx, args_json)
  };

  const char* result;
  JSValue result_js = JS_Call(ctx, function_wrapper, global, 1, args);
  failed = (JS_IsException(result_js) || JS_IsError(ctx, result_js));
  if (failed) {
    result = "Error calling function!";
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
  bool failed = (JS_IsException(val) || JS_IsError(ctx, val));
  const char* result;
  if (failed) {
    result = "Error in evaluation!";
  } else {
    result = JS_ValToJSON(ctx, &val);
  }

  JS_FreeValue(ctx, val);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  return result;
}
