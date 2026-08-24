#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#include <process.h>
#define GETPID() _getpid()
#else
#include <unistd.h>
#define GETPID() getpid()
#endif

void Rprintf(const char *, ...);
void REprintf(const char *, ...);
void Rvprintf(const char *, va_list);
void REvprintf(const char *, va_list);
void Rf_error(const char *, ...);

FILE* stdout_dummy = (FILE*)1;
FILE* stderr_dummy = (FILE*)2;

static _Thread_local int on_r_main_thread = 0;
static int main_pid = 0;

__attribute__((constructor))
static void libquickjs_record_main_thread(void) {
  on_r_main_thread = 1;
  main_pid = (int)GETPID();
}

static int safe_to_signal_r(void) {
  return on_r_main_thread && (int)GETPID() == main_pid;
}

#if defined(_WIN32)
#include <windows.h>
static void hard_terminate(int status) {
  TerminateProcess(GetCurrentProcess(), (UINT)status);
}
#elif defined(__linux__)
#include <sys/syscall.h>
static void hard_terminate(int status) {
  syscall(SYS_exit_group, status);
}
#else
#include <signal.h>
static void hard_terminate(int status) {
  (void)status;
  raise(SIGKILL);
}
#endif

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

int fileno_wrapper(FILE *stream) {
  if (stream == stdout_dummy) return 1;
  if (stream == stderr_dummy) return 2;
  return fileno(stream);
}

long ftell_wrapper(FILE *stream) {
  if (stream == stdout_dummy || stream == stderr_dummy) {
    errno = ESPIPE;
    return -1;
  }
  return ftell(stream);
}

int fseek_wrapper(FILE *stream, long offset, int whence) {
  if (stream == stdout_dummy || stream == stderr_dummy) {
    errno = ESPIPE;
    return -1;
  }
  return fseek(stream, offset, whence);
}

#if defined(__linux__) || defined(__GLIBC__)
off_t ftello_wrapper(FILE *stream) {
  if (stream == stdout_dummy || stream == stderr_dummy) {
    errno = ESPIPE;
    return -1;
  }
  return ftello(stream);
}

int fseeko_wrapper(FILE *stream, off_t offset, int whence) {
  if (stream == stdout_dummy || stream == stderr_dummy) {
    errno = ESPIPE;
    return -1;
  }
  return fseeko(stream, offset, whence);
}
#endif

int feof_wrapper(FILE *stream) {
  if (stream == stdout_dummy || stream == stderr_dummy) return 0;
  return feof(stream);
}

int ferror_wrapper(FILE *stream) {
  if (stream == stdout_dummy || stream == stderr_dummy) return 0;
  return ferror(stream);
}

void clearerr_wrapper(FILE *stream) {
  if (stream == stdout_dummy || stream == stderr_dummy) return;
  clearerr(stream);
}

size_t fread_wrapper(void *ptr, size_t size, size_t nitems, FILE *stream) {
  if (stream == stdout_dummy || stream == stderr_dummy) {
    errno = EBADF;
    return 0;
  }
  return fread(ptr, size, nitems, stream);
}

void exit_wrapper(int status) {
  if (safe_to_signal_r()) Rf_error("exit(%d) called from QuickJS", status);
  hard_terminate(status);
}

void _exit_wrapper(int status) {
  if (safe_to_signal_r()) Rf_error("exit(%d) called from QuickJS", status);
  hard_terminate(status);
}

void abort_wrapper(void) {
  if (safe_to_signal_r()) Rf_error("abort() called from QuickJS");
  hard_terminate(134); /* 128 + SIGABRT, the conventional shell exit code */
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
#define fileno fileno_wrapper
#define ftell ftell_wrapper
#define fseek fseek_wrapper
#if defined(__linux__) || defined(__GLIBC__)
#define ftello ftello_wrapper
#define fseeko fseeko_wrapper
#endif
#define feof feof_wrapper
#define ferror ferror_wrapper
#define clearerr clearerr_wrapper
#define fread fread_wrapper
#define _exit _exit_wrapper
#define exit exit_wrapper
#define abort abort_wrapper

#include "quickjs/dtoa.c"
#include "quickjs/libregexp.c"
#include "quickjs/libunicode.c"
#include "quickjs/quickjs.c"

int quickjsr_get_date_epoch_ms(JSContext *ctx, JSValueConst value, double *result) {
  if (!JS_IsDate(value)) return -1;
  return JS_ToFloat64(ctx, result, JS_VALUE_GET_OBJ(value)->u.object_data);
}

JSValue exit_wrapper_js(JSContext *ctx, int status) {
  if (safe_to_signal_r()) {
    JS_ThrowInternalError(ctx, "exit(%d) called from QuickJS", status);
    JSValue err = JS_GetException(ctx);
    JS_SetUncatchableError(ctx, err);
    return JS_Throw(ctx, err);
  }
  hard_terminate(status);
  return JS_UNDEFINED; /* unreachable */
}

#include "quickjs/quickjs-libc.c"
