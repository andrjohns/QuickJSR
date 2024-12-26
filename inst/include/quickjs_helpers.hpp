#ifndef QUICKJS_HELPERS_HPP
#define QUICKJS_HELPERS_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <quickjsr/JS_SEXP.hpp>

/**
 * These functions were adapted from the qjs.c file in the QuickJS source code.
*/
extern "C" int js__has_suffix(const char *str, const char *suffix);
#ifndef countof
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif

namespace quickjsr {
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
      cpp11::stop("Could not load '%s'\n", filename);
    }

    if (module < 0) {
      module = js__has_suffix(filename, ".mjs");
    }
    if (module) {
      eval_flags = JS_EVAL_TYPE_MODULE;
    } else {
      eval_flags = JS_EVAL_TYPE_GLOBAL;
    }
    ret = eval_buf(ctx, buf, buf_len, "<input>", eval_flags);
    js_free(ctx, (void*)buf);
    return ret;
  }

  /* also used to initialize the worker context */
  static JSContext* JS_NewCustomContext(JSRuntime *rt) {
    JSContext *ctx;
    ctx = JS_NewContext(rt);
    if (!ctx){
        return NULL;
    }

    /* system modules */
    js_init_module_std(ctx, "std");
    js_init_module_os(ctx, "os");

    JSValue proto = JS_NewObject(ctx);
    JS_SetClassProto(ctx, quickjsr::js_renv_class_id, proto);

    JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);

    js_init_module_os(ctx, "os");
    js_init_module_std(ctx, "std");

    js_std_add_helpers(ctx, 0, (char**)"");

    const char *str = "import * as std from 'std';\n"
        "import * as os from 'os';\n"
        "globalThis.std = std;\n"
        "globalThis.os = os;\n";
    eval_buf(ctx, str, strlen(str), "<input>", JS_EVAL_TYPE_MODULE);

    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue r_obj = quickjsr::create_r_object(ctx);
    JS_SetPropertyStr(ctx, global_obj, "R", r_obj);
    JS_FreeValue(ctx, global_obj);

    return ctx;
  }

  JSRuntime* JS_NewCustomRuntime(int stack_size) {
    JSRuntime *rt;
    rt = JS_NewRuntime();
    if (!rt){
        return NULL;
    }

    // Workaround for RStan stack overflow until they update
    if (stack_size != -1) {
      JS_SetMaxStackSize(rt, 0);
    }
    js_std_set_worker_new_context_func(JS_NewCustomContext);
    js_std_init_handlers(rt);
    // Initialise a class which can be used for passing SEXP objects to JS
    // without needing conversion
    JS_NewClass(rt, quickjsr::js_sexp_class_id, &quickjsr::js_sexp_class_def);
    JS_NewClass(rt, quickjsr::js_renv_class_id, &quickjsr::js_renv_class_def);

    return rt;
  }
}

#endif
