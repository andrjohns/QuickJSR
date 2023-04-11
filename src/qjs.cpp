#include <Rcpp.h>
#include <quickjs.h>
#include <fstream>
#include <sstream>

// Register the Rcpp external pointer types with the correct cleanup/finaliser functions
void JS_FreeContext(JSContext* ctx);
void JS_FreeRuntime(JSRuntime* ctx);
using ContextXPtr = Rcpp::XPtr<JSContext, Rcpp::PreserveStorage, JS_FreeContext>;
using RuntimeXPtr = Rcpp::XPtr<JSRuntime, Rcpp::PreserveStorage, JS_FreeRuntime>;

RcppExport SEXP qjs_context_(SEXP stack_size_) {
  int stack_size = Rcpp::as<int>(stack_size_);

  RuntimeXPtr rt(JS_NewRuntime());
  if (stack_size != -1) {
    JS_SetMaxStackSize(rt.get(), 4097152);
  }
  ContextXPtr ctx(JS_NewContext(rt));

  return Rcpp::List::create(
    Rcpp::Named("runtime_ptr") = rt,
    Rcpp::Named("context_ptr") = ctx
  );
}

RcppExport SEXP qjs_source_(SEXP ctx_ptr_, SEXP code_string_) {
  JSContext* ctx = ContextXPtr(ctx_ptr_);
  const char* code_string = Rcpp::as<const char*>(code_string_);

  JSValue val = JS_Eval(ctx, code_string, std::strlen(code_string), "", 0);
  bool succeeded = !JS_IsException(val);
  JS_FreeValue(ctx, val);

  return Rcpp::wrap(succeeded);
}

RcppExport SEXP qjs_validate_(SEXP ctx_ptr_, SEXP function_name_) {
  JSContext* ctx = ContextXPtr(ctx_ptr_);
  const char* function_name = Rcpp::as<const char*>(function_name_);

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue val = JS_GetPropertyStr(ctx, global, function_name);

  bool succeeded = !JS_IsException(val);
  JS_FreeValue(ctx, val);
  JS_FreeValue(ctx, global);

  return Rcpp::wrap(succeeded);
}

std::string JS_ValToJSON(JSContext* ctx, JSValue* val) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue json = JS_GetPropertyStr(ctx, global, "JSON");
  JSValue stringify = JS_GetPropertyStr(ctx, json, "stringify");

  JSValue result_js = JS_Call(ctx, stringify, global, 1, val);
  const char* json_string = JS_ToCString(ctx, result_js);
  std::string result(json_string);

  JS_FreeCString(ctx, json_string);
  JS_FreeValue(ctx, result_js);
  JS_FreeValue(ctx, stringify);
  JS_FreeValue(ctx, json);
  JS_FreeValue(ctx, global);

  return result;
}

RcppExport SEXP qjs_call_(SEXP ctx_ptr_, SEXP function_name_, SEXP args_json_) {
  JSContext* ctx = ContextXPtr(ctx_ptr_);
  std::string function_name = Rcpp::as<std::string>(function_name_);

  // Arguments are passed from R as a JSON object string, so we use a wrapper function
  // which 'spreads' the object to separate arguments.
  std::string wrapped_name = function_name + "_wrapper";
  std::string call_wrapper =
    "function " + wrapped_name + "(args_object) { return " + function_name +
    "(...Object.values(JSON.parse(args_object))); }";

  JSValue tmp = JS_Eval(ctx, call_wrapper.c_str(), std::strlen(call_wrapper.c_str()), "", 0);
  JS_FreeValue(ctx, tmp);

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue function_wrapper = JS_GetPropertyStr(ctx, global, wrapped_name.c_str());
  JSValue args[] = {
    JS_NewString(ctx, Rcpp::as<const char*>(args_json_))
  };

  JSValue result_js = JS_Call(ctx, function_wrapper, global, 1, args);
  std::string result = JS_ValToJSON(ctx, &result_js);

  JS_FreeValue(ctx, result_js);
  JS_FreeValue(ctx, args[0]);
  JS_FreeValue(ctx, function_wrapper);
  JS_FreeValue(ctx, global);

  return Rcpp::wrap(result);
}

RcppExport SEXP qjs_eval_(SEXP eval_string_) {
  std::string eval_string = Rcpp::as<std::string>(eval_string_);

  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  JSValue val = JS_Eval(ctx, eval_string.c_str(), std::strlen(eval_string.c_str()), "", 0);
  std::string result = JS_ValToJSON(ctx, &val);

  JS_FreeValue(ctx, val);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  return Rcpp::wrap(result);
}
