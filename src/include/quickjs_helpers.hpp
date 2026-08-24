#ifndef QUICKJS_HELPERS_HPP
#define QUICKJS_HELPERS_HPP

#include "quickjs.h"
#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <quickjsr/JS_SEXP.hpp>
#include <cstdint>
#include <memory>
#include <string_view>

/**
 * These functions were adapted from the qjs.c file in the QuickJS source code.
*/
static inline bool js__has_suffix(std::string_view value,
                                  std::string_view suffix) {
    return value.size() >= suffix.size() &&
      value.substr(value.size() - suffix.size()) == suffix;
}

namespace quickjsr {
  enum ContextProfile {
    CONTEXT_BARE,
    CONTEXT_STANDARD,
    CONTEXT_HOST
  };

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
      JSValue exc = JS_GetException(ctx);
      const char* res_str = JS_ToCString(ctx, exc);
      std::string msg = res_str;
      JS_FreeCString(ctx, res_str);
      std::string stack = "";
      if (JS_IsError(exc)) {
        JSValue stack_val = JS_GetPropertyStr(ctx, exc, "stack");
        const char* stack_str = JS_ToCString(ctx, stack_val);
        stack = stack_str;
        stack = "\n" + stack;
        JS_FreeCString(ctx, stack_str);
        JS_FreeValue(ctx, stack_val);
      }
      JS_FreeValue(ctx, exc);
      JS_FreeValue(ctx, val);
      cpp11::stop("JavaScript Exception: \n" + msg + stack);
      ret = -1;
    } else {
      JS_FreeValue(ctx, val);
      ret = 0;
    }
    return ret;
  }

  static int eval_file(JSContext *ctx, const char *filename, int module) {
    int ret, eval_flags;
    size_t buf_len;

    uint8_t* raw_buffer = js_load_file(ctx, &buf_len, filename);
    if (!raw_buffer) {
      cpp11::stop("Could not load '%s'\n", filename);
    }
    auto free_buffer = [ctx](uint8_t* buffer) { js_free(ctx, buffer); };
    std::unique_ptr<uint8_t, decltype(free_buffer)> buffer(
      raw_buffer, free_buffer
    );

    if (module < 0) {
      module = js__has_suffix(filename, ".mjs");
    }
    if (module) {
      eval_flags = JS_EVAL_TYPE_MODULE;
    } else {
      eval_flags = JS_EVAL_TYPE_GLOBAL;
    }
    ret = eval_buf(
      ctx, reinterpret_cast<const char*>(buffer.get()), buf_len, "<input>",
      eval_flags
    );
    return ret;
  }

  static JSContext* JS_NewProfileContext(JSRuntime *rt, int profile) {
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

    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue array_ctor = JS_GetPropertyStr(ctx, global_obj, "Array");
    JSValue array_proto = JS_GetPropertyStr(ctx, array_ctor, "prototype");
    JSValue view_proto = JS_NewObjectProto(ctx, array_proto);
    JS_SetPropertyFunctionList(
      ctx, view_proto, quickjsr::js_rvector_view_funcs,
      static_cast<int>(std::size(quickjsr::js_rvector_view_funcs))
    );
    JS_SetClassProto(ctx, quickjsr::js_rvector_view_class_id, view_proto);
    JSValue masked_proto = JS_NewObject(ctx);
    JS_SetClassProto(
      ctx, quickjsr::js_masked_typed_array_class_id, masked_proto
    );
    JS_FreeValue(ctx, array_proto);
    JS_FreeValue(ctx, array_ctor);
    JS_FreeValue(ctx, global_obj);

    if (profile == CONTEXT_BARE) {
      return ctx;
    }

    JS_SetModuleLoaderFunc2(rt, NULL, js_module_loader, js_module_check_attributes, NULL);

    js_std_add_helpers(ctx, 0, (char**)"");

    const char *str = "import * as std from 'std';\n"
        "import * as os from 'os';\n"
        "globalThis.std = std;\n"
        "globalThis.os = os;\n"
        // console.log is defined by js_std_add_helpers(); console.error is
        // not, so add it here, writing to stderr instead of stdout.
        "globalThis.console.error = function(...args) {\n"
        "  std.err.puts(args.join(' ') + '\\n');\n"
        "};\n";
    eval_buf(ctx, str, strlen(str), "<input>", JS_EVAL_TYPE_MODULE);

    if (profile == CONTEXT_HOST) {
      global_obj = JS_GetGlobalObject(ctx);
      JSValue r_obj = quickjsr::create_r_object(ctx);
      JS_SetPropertyStr(ctx, global_obj, "R", r_obj);
      JS_FreeValue(ctx, global_obj);
    }

    return ctx;
  }

  static JSContext* JS_NewWorkerContext(JSRuntime *rt) {
    int profile = static_cast<int>(reinterpret_cast<intptr_t>(
      JS_GetRuntimeOpaque(rt)
    ));
    return JS_NewProfileContext(rt, profile);
  }

  inline JSRuntime* JS_NewProfileRuntime(int stack_size, int profile) {
    JSRuntime *rt;
    rt = JS_NewRuntime();
    if (!rt){
        return NULL;
    }

    if (stack_size != -1) {
      JS_SetMaxStackSize(rt, stack_size);
    }
    JS_SetRuntimeOpaque(
      rt, reinterpret_cast<void*>(static_cast<intptr_t>(profile))
    );
    if (profile != CONTEXT_BARE) {
      js_std_set_worker_new_context_func(JS_NewWorkerContext);
      js_std_init_handlers(rt);
    }

    JS_NewClassID(rt, &quickjsr::js_sexp_class_id);
    JS_NewClassID(rt, &quickjsr::js_renv_class_id);
    JS_NewClassID(rt, &quickjsr::js_rvector_view_class_id);
    JS_NewClassID(rt, &quickjsr::js_masked_typed_array_class_id);
    // Initialise a class which can be used for passing SEXP objects to JS
    // without needing conversion
    JS_NewClass(rt, quickjsr::js_sexp_class_id, &quickjsr::js_sexp_class_def);
    JS_NewClass(rt, quickjsr::js_renv_class_id, &quickjsr::js_renv_class_def);
    JS_NewClass(
      rt, quickjsr::js_rvector_view_class_id,
      &quickjsr::js_rvector_view_class_def
    );
    JS_NewClass(
      rt, quickjsr::js_masked_typed_array_class_id,
      &quickjsr::js_masked_typed_array_class_def
    );

    return rt;
  }

  static JSContext* JS_NewCustomContext(JSRuntime *rt) {
    return JS_NewProfileContext(rt, CONTEXT_HOST);
  }

  inline JSRuntime* JS_NewCustomRuntime(int stack_size) {
    return JS_NewProfileRuntime(stack_size, CONTEXT_HOST);
  }
}

#endif
