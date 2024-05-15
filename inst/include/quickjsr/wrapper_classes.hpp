#ifndef QUICKJSR_WRAPPER_CLASSES_HPP
#define QUICKJSR_WRAPPER_CLASSES_HPP

#include <quickjs-libc.h>

namespace quickjsr {

struct JSRuntimeContextWrapper {
  JSRuntime* rt;
  JSContext* ctx;

  JSRuntimeContextWrapper() : rt(JS_NewRuntime()), ctx(JS_NewContext(rt)) {
    js_std_init_handlers(rt);
  }

  JSRuntimeContextWrapper(int stack_size) : rt(JS_NewRuntime()), ctx(JS_NewContext(rt)) {
    if (stack_size != -1) {
      JS_SetMaxStackSize(rt, stack_size);
    }
    js_std_init_handlers(rt);
  }

  ~JSRuntimeContextWrapper() {
    JS_FreeContext(ctx);
    js_std_free_handlers(rt);
    JS_FreeRuntime(rt);
  }
};
} // namespace quickjsr

#endif
