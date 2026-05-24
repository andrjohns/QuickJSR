#include <stdio.h>
#include <stdarg.h>

void Rprintf(const char *, ...);
void REprintf(const char *, ...);
void Rvprintf(const char *, va_list);
void REvprintf(const char *, va_list);
void Rf_error(const char *, ...);

FILE* stdout_dummy = (FILE*)1;
FILE* stderr_dummy = (FILE*)2;

unsigned long fwrite_wrapper(const void * __ptr, size_t __size, size_t __nitems,
                     FILE * __stream) {
  if (__stream == stdout_dummy) {
    Rprintf("%.*s", (int)(__size * __nitems), (const char*)__ptr);
    return __nitems;
  } else if (__stream == stderr_dummy) {
    REprintf("%.*s", (int)(__size * __nitems), (const char*)__ptr);
    return __nitems;
  } else {
    return fwrite(__ptr, __size, __nitems, __stream);
  }
}

int fputs_wrapper(const char *s, FILE *stream) {
  if (stream == stdout_dummy) {
    Rprintf("%s", s);
    return 0;
  } else if (stream == stderr_dummy) {
    REprintf("%s", s);
    return 0;
  } else {
    return fputs(s, stream);
  }
}

int putchar_wrapper(int c) {
  char buf[2] = { (char)c, '\0' };
  Rprintf("%s", buf);
  return c;
}

int fputc_wrapper(int c, FILE *stream) {
  if (stream == stdout_dummy) {
    char buf[2] = { (char)c, '\0' };
    Rprintf("%s", buf);
    return c;
  } else if (stream == stderr_dummy) {
    char buf[2] = { (char)c, '\0' };
    REprintf("%s", buf);
    return c;
  } else {
    return fputc(c, stream);
  }
}

int fprintf_wrapper(FILE *stream, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int rtn = 0;

  if (stream == stdout_dummy) {
    Rvprintf(format, args);
  } else if (stream == stderr_dummy) {
    REvprintf(format, args);
  } else {
    rtn = vfprintf(stream, format, args);
  }
  va_end(args);
  return rtn;
}

int fflush_wrapper(FILE *stream) {
  if (stream == stdout_dummy || stream == stderr_dummy) {
    return 0;
  } else {
    return fflush(stream);
  }
}

int puts_wrapper(const char *s) {
  Rprintf("%s\n", s);
  return 0;
}

int printf_wrapper(const char *format, ...) {
  va_list args;
  va_start(args, format);
  Rvprintf(format, args);
  va_end(args);
  return 0;
}

void exit_wrapper(int status) {
  Rf_error("exit(%d) called from QuickJS", status);
}

void abort_wrapper() {
  Rf_error("abort() called from QuickJS");
}

#define stdout stdout_dummy
#define stderr stderr_dummy
#define fwrite fwrite_wrapper
#define fputs fputs_wrapper
#define putchar putchar_wrapper
#define fputc fputc_wrapper
#define fprintf fprintf_wrapper
#define fflush fflush_wrapper
#define puts puts_wrapper
#define printf printf_wrapper
#define _exit exit_wrapper
#define exit exit_wrapper
#define abort abort_wrapper

#include "quickjs/dtoa.c"
#include "quickjs/libregexp.c"
#include "quickjs/libunicode.c"
#include "quickjs/quickjs.c"
#include "quickjs/quickjs-libc.c"
