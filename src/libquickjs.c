#include <stdio.h>

void Rf_error(const char* fmt, ...);

void Rf_abort(void) {
  Rf_error("Aborted");
}
void Rf_exit(int status) {
  Rf_error("Exited with status %d", status);
}

#define abort Rf_abort
#define exit Rf_exit

#include "quickjs/cutils.c"
#include "quickjs/libbf.c"
#include "quickjs/libregexp.c"
#include "quickjs/libunicode.c"
#include "quickjs/quickjs.c"
#include "quickjs/quickjs-libc.c"
