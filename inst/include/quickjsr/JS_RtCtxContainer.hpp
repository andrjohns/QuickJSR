#ifndef QUICKJSR_JS_RTCTXCONTAINER_HPP
#define QUICKJSR_JS_RTCTXCONTAINER_HPP

#include <quickjs-libc.h>
#include <quickjs_helpers.hpp>

namespace quickjsr {

class JS_RtCtxContainer {
  public:
    JSRuntime* rt;
    JSContext* ctx;

    JS_RtCtxContainer(int stack_size = 0) {
      rt = quickjsr::JS_NewCustomRuntime(stack_size);
      ctx = quickjsr::JS_NewCustomContext(rt);
    }

    ~JS_RtCtxContainer() {
      JS_FreeContext(ctx);
      js_std_free_handlers(rt);
      JS_FreeRuntime(rt);
    }
};

} // namespace quickjsr

#endif
