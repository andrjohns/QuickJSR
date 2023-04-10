#include <Rcpp.h>
#include <quickjs.h>

RcppExport SEXP qjs_eval_(SEXP eval_string_) {
  std::string eval_string = "JSON.stringify(" + Rcpp::as<std::string>(eval_string_) + ")";

  JSRuntime *rt = JS_NewRuntime();
  JSContext *ctx = JS_NewContext(rt);

  JSValue val = JS_Eval(ctx, eval_string.c_str(), std::strlen(eval_string.c_str()), "", 0);
  const char* result = JS_ToCString(ctx, val);

  JS_FreeValue(ctx, val);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  return Rcpp::wrap(result);
}
