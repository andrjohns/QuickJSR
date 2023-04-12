#include <Rcpp.h>
#include <fstream>
#include <sstream>

typedef struct JSValue JSvalue;
typedef struct JSRuntime JSRuntime;
typedef struct JSContext JSContext;

// We compile the functions as a separate C translation unit, as the QuickJS
// C headers trigger -Wpedantic warnings under C++
#ifdef __cplusplus
extern "C" {
#endif

  JSRuntime* JS_NewRuntime(void);
  void JS_SetMaxStackSize(JSRuntime* rt, size_t stack_size);
  JSContext* JS_NewContext(JSRuntime* rt);
  void JS_FreeContext(JSContext* ctx);
  void JS_FreeRuntime(JSRuntime* rt);

  bool qjs_source_impl(JSContext* ctx, const char* code_string);
  bool qjs_validate_impl(JSContext* ctx, const char* function_name);
  const char* qjs_call_impl(JSContext* ctx, const char* wrapped_name,
                        const char* call_wrapper, const char* args_json);
  const char* qjs_eval_impl(const char* eval_string);

#ifdef __cplusplus
}
#endif

// Register the Rcpp external pointer types with the correct cleanup/finaliser functions
using ContextXPtr = Rcpp::XPtr<JSContext, Rcpp::PreserveStorage, JS_FreeContext>;
using RuntimeXPtr = Rcpp::XPtr<JSRuntime, Rcpp::PreserveStorage, JS_FreeRuntime>;

RcppExport SEXP qjs_context_(SEXP stack_size_) {
  int stack_size = Rcpp::as<int>(stack_size_);

  RuntimeXPtr rt(JS_NewRuntime());
  if (stack_size != -1) {
    JS_SetMaxStackSize(rt.get(), stack_size);
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
  bool succeeded = qjs_source_impl(ctx, code_string);

  return Rcpp::wrap(succeeded);
}

RcppExport SEXP qjs_validate_(SEXP ctx_ptr_, SEXP function_name_) {
  JSContext* ctx = ContextXPtr(ctx_ptr_);
  const char* function_name = Rcpp::as<const char*>(function_name_);
  bool succeeded = qjs_validate_impl(ctx, function_name);

  return Rcpp::wrap(succeeded);
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

  std::string result = qjs_call_impl(ctx, wrapped_name.c_str(), call_wrapper.c_str(),
                                      Rcpp::as<const char*>(args_json_));

  return Rcpp::wrap(result);
}

RcppExport SEXP qjs_eval_(SEXP eval_string_) {
  const char* eval_string = Rcpp::as<const char*>(eval_string_);
  std::string result = qjs_eval_impl(eval_string);

  return Rcpp::wrap(result);
}
