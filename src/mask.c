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

FILE* get_con(int which) {
  if (which == 0) {
    return stdout;
  } else {
    return stderr;
  }
}
