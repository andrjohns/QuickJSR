#include <cpp11.hpp>
#include <R_ext/Visibility.h>

#ifdef __cplusplus
extern "C" {
#endif

  SEXP qjs_context_(SEXP stack_size_);
  SEXP qjs_source_(SEXP ctx_ptr_, SEXP code_string_);
  SEXP qjs_validate_(SEXP ctx_ptr_, SEXP code_string_);
  SEXP qjs_call_(SEXP ctx_ptr_, SEXP function_name_, SEXP args_json_);
  SEXP qjs_eval_(SEXP eval_string_);
  SEXP to_json_(SEXP arg_, SEXP auto_unbox_);
  SEXP from_json_(SEXP json_);


  static const R_CallMethodDef CallEntries[] = {
    {"qjs_call_",     (DL_FUNC) &qjs_call_,     3},
    {"qjs_context_",  (DL_FUNC) &qjs_context_,  1},
    {"qjs_eval_",     (DL_FUNC) &qjs_eval_,     1},
    {"qjs_source_",   (DL_FUNC) &qjs_source_,   2},
    {"qjs_validate_", (DL_FUNC) &qjs_validate_, 2},
    {"to_json_", (DL_FUNC) &to_json_, 2},
    {"from_json_", (DL_FUNC) &from_json_, 1},
    {NULL, NULL, 0}
  };

#ifdef __cplusplus
}
#endif

extern "C" attribute_visible void R_init_QuickJSR(DllInfo* dll){
  R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
}
