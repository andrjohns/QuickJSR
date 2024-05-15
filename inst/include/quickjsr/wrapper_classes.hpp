#ifndef QUICKJSR_WRAPPER_CLASSES_HPP
#define QUICKJSR_WRAPPER_CLASSES_HPP

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

  JSRuntimeContextWrapper(int stack_size = -1) :
    rt(InitRuntime(stack_size)), ctx(JS_NewContext(rt)) { }

  ~JSRuntimeContextWrapper() {
    JS_FreeContext(ctx);
    js_std_free_handlers(rt);
    JS_FreeRuntime(rt);
  }
};

JSContext* current_context = nullptr;

struct ExecutionScope {
  ExecutionScope(JSContext* ctx) { current_context = ctx; }
  ~ExecutionScope() { current_context = nullptr; }
};

struct JSValueWrapper {
  JSValue val;

  JSValueWrapper(JSValue val) : val(val) { }
  operator JSValue() const { return val; }
  JSValue* operator&() { return &val; }
  ~JSValueWrapper() { JS_FreeValue(current_context, val); }
};
} // namespace quickjsr

#endif
