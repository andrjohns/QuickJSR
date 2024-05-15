#ifndef QUICKJSR_WRAPPER_CLASSES_HPP
#define QUICKJSR_WRAPPER_CLASSES_HPP

#include <string>
#include <quickjs-libc.h>

namespace quickjsr {

JSRuntime* InitRuntime(int stack_size) {
  JSRuntime* rt = JS_NewRuntime();
  if (stack_size != -1) {
    JS_SetMaxStackSize(rt, stack_size);
  }
  js_std_init_handlers(rt);
  return rt;
}

struct JSRuntimeContextWrapper {
  JSRuntime* rt;
  JSContext* ctx;
  JSValue global_obj;

  JSRuntimeContextWrapper(int stack_size = -1) :
    rt(InitRuntime(stack_size)),
    ctx(JS_NewContext(rt)),
    global_obj(JS_GetGlobalObject(ctx)) { }

  ~JSRuntimeContextWrapper() {
    JS_FreeValue(ctx, global_obj);
    JS_FreeContext(ctx);
    js_std_free_handlers(rt);
    JS_FreeRuntime(rt);
  }

  JSValue Eval(const std::string& code, int flags = JS_EVAL_TYPE_GLOBAL) {
    return JS_Eval(ctx, code.c_str(), code.length(), "<eval>", flags);
  }
  JSValue EvalFunctionGlobal(const std::string& fun_name, JSValue* args, int n_args) {
    JSValue fun = JS_GetPropertyStr(ctx, global_obj, fun_name.c_str());
    JSValue res = JS_Call(ctx, fun, global_obj, n_args, args);
    JS_FreeValue(ctx, fun);
    return res;
  }
};

JSRuntimeContextWrapper* current_context = nullptr;

struct ExecutionScope {
  ExecutionScope(JSRuntimeContextWrapper* ctx) {
    if (current_context != nullptr) {
      JS_ThrowInternalError(ctx->ctx, "ExecutionScope already exists");
    }
    current_context = ctx;
  }
  ~ExecutionScope() { current_context = nullptr; }
};

struct JSValueWrapper {
  JSValue val;

  JSValueWrapper(JSValue val) : val(val) { }
  operator JSValue() const { return val; }
  JSValue* operator&() { return &val; }
  ~JSValueWrapper() { JS_FreeValue(current_context->ctx, val); }
};
} // namespace quickjsr

#endif
