#include <stdio.h>

void Rf_error(const char *, ...);
void Rprintf(const char *, ...);
void Rf_abort(void) {
  Rf_error("Aborted");
}
void Rf_exit(int status) {
  Rf_error("Exit with status %d", status);
}
void Rf_putchar(int c) {
  Rprintf("%c", c);
}

#define abort Rf_abort
#define putchar Rf_putchar
#define _putchar Rf_putchar
#define exit Rf_exit
#define _exit Rf_exit
#define printf Rprintf

#include "quickjs/cutils.c"
#include "quickjs/libbf.c"
#include "quickjs/libregexp.c"
#include "quickjs/libunicode.c"
#include "quickjs/quickjs.c"
#include "quickjs/quickjs-libc.c"
