#ifndef QUICKJS_HELPERS_HPP
#define QUICKJS_HELPERS_HPP

#include <cutils.h>
#include <quickjs-libc.h>

/**
 * These functions were extracted from the qjs.c file in the QuickJS source code.
*/

static int eval_buf(JSContext *ctx, const char* buf, int buf_len,
                    const char *filename, int eval_flags) {
  JSValue val;
  int ret;

  if ((eval_flags & JS_EVAL_TYPE_MASK) == JS_EVAL_TYPE_MODULE) {
    /* for the modules, we compile then run to be able to set
        import.meta */
    val = JS_Eval(ctx, buf, buf_len, filename,
                  eval_flags | JS_EVAL_FLAG_COMPILE_ONLY);
    if (!JS_IsException(val)) {
      js_module_set_import_meta(ctx, val, TRUE, TRUE);
      val = JS_EvalFunction(ctx, val);
    }
  } else {
    val = JS_Eval(ctx, buf, buf_len, filename, eval_flags);
  }
  if (JS_IsException(val)) {
    js_std_dump_error(ctx);
    ret = -1;
  } else {
    ret = 0;
  }
  JS_FreeValue(ctx, val);
  return ret;
}

static int eval_file(JSContext *ctx, const char *filename, int module) {
  const char* buf;
  int ret, eval_flags;
  size_t buf_len;

  buf = (const char*)js_load_file(ctx, &buf_len, filename);
  if (!buf) {
    perror(filename);
    exit(1);
  }

  if (module < 0) {
    module = (has_suffix(filename, ".mjs") || JS_DetectModule((const char *)buf, buf_len));
  }
  if (module) {
    eval_flags = JS_EVAL_TYPE_MODULE;
  } else {
    eval_flags = JS_EVAL_TYPE_GLOBAL;
  }
  ret = eval_buf(ctx, buf, buf_len, filename, eval_flags);
  js_free(ctx, (void*)buf);
  return ret;
}

/* also used to initialize the worker context */
static JSContext *JS_NewCustomContext(JSRuntime *rt) {
  JSContext *ctx;
  ctx = JS_NewContext(rt);
  if (!ctx){
      return NULL;
  }

  JS_AddIntrinsicBigFloat(ctx);
  JS_AddIntrinsicBigDecimal(ctx);
  JS_AddIntrinsicOperators(ctx);
  JS_EnableBignumExt(ctx, TRUE);

  /* system modules */
  js_init_module_std(ctx, "std");
  js_init_module_os(ctx, "os");
  return ctx;
}


#endif
