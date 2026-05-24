#include <stdio.h>

void Rprintf(const char *, ...);

FILE* stdout_dummy;


unsigned long fwrite_wrapper(const void * __ptr, size_t __size, size_t __nitems,
                     FILE * __stream) {
  if (__stream == stdout_dummy) {
    Rprintf("%.*s", (int)(__size * __nitems), (const char*)__ptr);
    return __nitems;
  } else {
    return fwrite(__ptr, __size, __nitems, __stream);
  }
}

#define stdout stdout_dummy
#define fwrite fwrite_wrapper

#include "quickjs/dtoa.c"
#include "quickjs/libregexp.c"
#include "quickjs/libunicode.c"
#include "quickjs/quickjs.c"
#include "quickjs/quickjs-libc.c"
