#include "quickjsr.cpp"

#include <R_ext/Visibility.h>

extern "C" {
  static const R_CallMethodDef CallEntries[] = {
    {"qjs_call_",     (DL_FUNC) &qjs_call_,     3},
    {"qjs_context_",  (DL_FUNC) &qjs_context_,  1},
    {"qjs_eval_",     (DL_FUNC) &qjs_eval_,     1},
    {"qjs_source_",   (DL_FUNC) &qjs_source_,   3},
    {"qjs_validate_", (DL_FUNC) &qjs_validate_, 2},
    {"qjs_get_",      (DL_FUNC) &qjs_get_,      2},
    {"qjs_assign_",   (DL_FUNC) &qjs_assign_,   3},
    {"to_json_",      (DL_FUNC) &to_json_,      2},
    {"from_json_",    (DL_FUNC) &from_json_,    1},
    {"qjs_version_",  (DL_FUNC) &qjs_version_,  0},
    {NULL, NULL, 0}
  };

  attribute_visible void R_init_QuickJSR(DllInfo* dll){
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
  }
}
