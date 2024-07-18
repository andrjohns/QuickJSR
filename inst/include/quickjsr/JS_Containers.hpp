#ifndef QUICKJSR_JS_CONTAINERS_HPP
#define QUICKJSR_JS_CONTAINERS_HPP

#include <cpp11/external_pointer.hpp>
#include <quickjs-libc.h>
#include <quickjs_helpers.hpp>

namespace quickjsr {

struct JS_RtCtxContainer {
  public:
    JSRuntime* rt;
    JSContext* ctx;

    JS_RtCtxContainer(int stack_size = 0)
      : rt(JS_NewCustomRuntime(stack_size)), ctx(JS_NewCustomContext(rt)) {}

    ~JS_RtCtxContainer() {
      JS_FreeContext(ctx);
      js_std_free_handlers(rt);
      JS_FreeRuntime(rt);
    }
};

struct RtCtxXPtr : cpp11::external_pointer<JS_RtCtxContainer> {
  using cpp11::external_pointer<JS_RtCtxContainer>::external_pointer;

  RtCtxXPtr(int stack_size = 0)
    : cpp11::external_pointer<JS_RtCtxContainer>(new JS_RtCtxContainer(stack_size)) {}

  operator JSContext*() const {
    return get()->ctx;
  }
};

struct JS_ValContainer {
  public:
    RtCtxXPtr rt_ctx;
    JSValue val;

    template <typename JSValT>
    JS_ValContainer(RtCtxXPtr in_rt_ctx, JSValT&& in_val)
      : rt_ctx(in_rt_ctx), val(std::forward<JSValT>(in_val)) {}

    ~JS_ValContainer() {
      JS_FreeValue(rt_ctx->ctx, val);
    }

    // Operator for assigning JS_ValContainer to JSValue
    operator JSValue() const {
      return val;
    }
};

} // namespace quickjsr

#endif
