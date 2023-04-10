#include <Rcpp.h>
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>
#include <Rversion.h>

using namespace Rcpp;

#ifdef __cplusplus
extern "C"  {
#endif

SEXP qjs_eval_(SEXP eval_string_);

#ifdef __cplusplus
}
#endif


#define CALLDEF(name, n)  {#name, (DL_FUNC) &name, n}

static const R_CallMethodDef CallEntries[] = {
  CALLDEF(qjs_eval_, 1),
  {NULL, NULL, 0}
};

#ifdef __cplusplus
extern "C"  {
#endif
void attribute_visible R_init_QuickJSR(DllInfo *dll) {
  R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  // The call to R_useDynamicSymbols indicates that if the correct C
  // entry point is not found in the shared library, then an error
  // should be signaled.  Currently, the default
  // behavior in R is to search all other loaded shared libraries for the
  // symbol, which is fairly dangerous behavior.  If you have registered
  // all routines in your library, then you should set this to FALSE
  // as done in the stats package. [copied from `R Programming for
  // Bioinformatics' // by Robert Gentleman]
}
#ifdef __cplusplus
}
#endif
