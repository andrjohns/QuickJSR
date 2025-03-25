void Rf_error(const char *, ...);
void Rf_abort(void) {
  Rf_error("Aborted");
}
void Rf_exit(int status) {
  Rf_error("Exit with status %d", status);
}

#define abort Rf_abort
#define _exit Rf_exit

#include "quickjs/cutils.c"
#include "quickjs/libbf.c"
#include "quickjs/libregexp.c"
#include "quickjs/libunicode.c"
#include "quickjs/quickjs.c"
#include "quickjs/quickjs-libc.c"
