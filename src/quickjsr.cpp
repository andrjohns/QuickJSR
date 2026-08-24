#include <cpp11.hpp>
#include <cpp11/declarations.hpp>
#include <quickjs-libc.h>
#include <quickjs_helpers.hpp>
#include <quickjsr.hpp>
#include <utility>

// Bundles the runtime and its context so a single external pointer owns both,
// guaranteeing the context is always freed before the runtime it depends on,
// regardless of R's garbage collection order.
struct QjsRuntimeContext {
  JSContext* ctx;
  JSRuntime* rt;
};

void JS_FreeRuntimeContext(QjsRuntimeContext* handle) {
  if (!handle) return;
  JS_FreeContext(handle->ctx);
  js_std_free_handlers(handle->rt);
  JS_FreeRuntime(handle->rt);
  delete handle;
}

struct ScopedRuntime {
  JSRuntime* rt;
  JSContext* ctx;
  bool has_std_handlers;
  ~ScopedRuntime() {
    if (ctx) JS_FreeContext(ctx);
    if (has_std_handlers && rt) js_std_free_handlers(rt);
    if (rt) JS_FreeRuntime(rt);
  }
  void release() {
    rt = nullptr;
    ctx = nullptr;
    has_std_handlers = false;
  }
};

struct ScopedJSValue {
  JSContext* ctx;
  JSValue value;

  ScopedJSValue(JSContext* ctx_, JSValue value_) : ctx(ctx_), value(value_) {}
  ~ScopedJSValue() { JS_FreeValue(ctx, value); }

  JSValue get() const { return value; }
  JSValue release() {
    return std::exchange(value, JS_UNDEFINED);
  }
};

struct ScopedJSValues {
  JSContext* ctx;
  std::vector<JSValue> values;

  explicit ScopedJSValues(JSContext* ctx_) : ctx(ctx_) {}
  ~ScopedJSValues() {
    for (JSValue value : values) JS_FreeValue(ctx, value);
  }

  void push(JSValue value) { values.push_back(value); }
  int size() const { return static_cast<int>(values.size()); }
  JSValue* data() { return values.data(); }
};

struct ScopedCString {
  JSContext* ctx;
  const char* value;

  ScopedCString(JSContext* ctx_, const char* value_) : ctx(ctx_), value(value_) {}
  ~ScopedCString() {
    if (value) JS_FreeCString(ctx, value);
  }
};

// Register the cpp11 external pointer type with the correct cleanup/finaliser function
using ContextXPtr = cpp11::external_pointer<QjsRuntimeContext, JS_FreeRuntimeContext>;

void JS_FreeValueRef(quickjsr::JSValueRefData* ref) {
  if (!ref) return;
  JS_FreeValue(ref->ctx, ref->value);
  JS_FreeValue(ref->ctx, ref->receiver);
  delete ref;
}

using ValueXPtr = cpp11::external_pointer<quickjsr::JSValueRefData, JS_FreeValueRef>;

SEXP make_value_ref(SEXP context_ptr, JSContext* ctx, ScopedJSValue& value,
                    ScopedJSValue& receiver) {
  quickjsr::JSValueRefData* data = new quickjsr::JSValueRefData{
    ctx, value.release(), receiver.release()
  };
  ValueXPtr handle(data);
  SEXP result = PROTECT(cpp11::as_sexp(handle));
  R_SetExternalPtrProtected(result, context_ptr);
  Rf_classgets(result, Rf_mkString("JSValueRef"));
  UNPROTECT(1);
  return result;
}

extern "C" {
  SEXP qjs_context_(SEXP stack_size_) {
    BEGIN_CPP11
    int stack_size;
    if (Rf_isInteger(stack_size_)) {
      stack_size = INTEGER_ELT(stack_size_, 0);
    } else if (Rf_isReal(stack_size_)) {
      stack_size = static_cast<int>(REAL_ELT(stack_size_, 0));
    } else {
      cpp11::stop("stack_size must be integer or numeric");
    }
    JSRuntime* rt = quickjsr::JS_NewCustomRuntime(stack_size);
    if (!rt) cpp11::stop("Could not create QuickJS runtime");
    ScopedRuntime guard{rt, nullptr, true};
    JSContext* ctx = quickjsr::JS_NewCustomContext(rt);
    guard.ctx = ctx;
    if (!ctx) cpp11::stop("Could not create QuickJS context");
    ContextXPtr handle(new QjsRuntimeContext{ctx, rt});
    guard.release();

    return cpp11::as_sexp(handle);
    END_CPP11
  }

  SEXP qjs_source_(SEXP ctx_ptr_, SEXP input_, SEXP is_file_) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);
    int ret;
    const char* input = Rf_translateCharUTF8(STRING_ELT(input_, 0));
    if (!Rf_isLogical(is_file_)) {
      cpp11::stop("is_file must be a logical value");
    }
    if (LOGICAL_ELT(is_file_, 0)) {
      ret = quickjsr::eval_file(ctx->ctx, input, -1);
    } else {
      ret = quickjsr::eval_buf(ctx->ctx, input, strlen(input), "<input>", JS_EVAL_TYPE_GLOBAL);
    }
    return cpp11::as_sexp(!ret);
    END_CPP11
  }

  SEXP qjs_validate_(SEXP ctx_ptr_, SEXP code_string_) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);
    const char* code_string = Rf_translateCharUTF8(STRING_ELT(code_string_, 0));
    ScopedJSValue value(
      ctx->ctx,
      JS_Eval(ctx->ctx, code_string, strlen(code_string), "<input>",
              JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY)
    );
    if (JS_IsException(value.get())) {
      ScopedJSValue exception(ctx->ctx, JS_GetException(ctx->ctx));
      return cpp11::as_sexp(false);
    }
    return cpp11::as_sexp(true);
    END_CPP11
  }

  SEXP qjs_call_(SEXP ctx_ptr_, SEXP fun_name_, SEXP args_list_) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);

    R_xlen_t n_args = Rf_xlength(args_list_);
    ScopedJSValues args(ctx->ctx);
    args.values.reserve(static_cast<size_t>(n_args));
    for (R_xlen_t i = 0; i < n_args; i++) {
      args.push(quickjsr::SEXP_to_JSValue(ctx->ctx, VECTOR_ELT(args_list_, i), true));
    }

    ScopedJSValue global(ctx->ctx, JS_GetGlobalObject(ctx->ctx));
    JSValue receiver_value = JS_UNDEFINED;
    ScopedJSValue fun(
      ctx->ctx,
      quickjsr::JS_GetPropertyRecursive(
        ctx->ctx, global.get(), Rf_translateCharUTF8(STRING_ELT(fun_name_, 0)),
        &receiver_value
      )
    );
    ScopedJSValue receiver(ctx->ctx, receiver_value);
    if (JS_IsException(fun.get())) {
      return quickjsr::JSValue_to_SEXP(ctx->ctx, fun.get());
    }
    ScopedJSValue result(
      ctx->ctx,
      JS_Call(ctx->ctx, fun.get(), receiver.get(), args.size(), args.data())
    );
    return quickjsr::JSValue_to_SEXP(ctx->ctx, result.get());
    END_CPP11
  }

  SEXP qjs_call_ref_(SEXP ctx_ptr_, SEXP fun_name_, SEXP args_list_) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);
    R_xlen_t n_args = Rf_xlength(args_list_);
    ScopedJSValues args(ctx->ctx);
    args.values.reserve(static_cast<size_t>(n_args));
    for (R_xlen_t i = 0; i < n_args; i++) {
      args.push(quickjsr::SEXP_to_JSValue(ctx->ctx, VECTOR_ELT(args_list_, i), true));
    }

    ScopedJSValue global(ctx->ctx, JS_GetGlobalObject(ctx->ctx));
    JSValue receiver_value = JS_UNDEFINED;
    ScopedJSValue fun(
      ctx->ctx,
      quickjsr::JS_GetPropertyRecursive(
        ctx->ctx, global.get(), Rf_translateCharUTF8(STRING_ELT(fun_name_, 0)),
        &receiver_value
      )
    );
    ScopedJSValue receiver(ctx->ctx, receiver_value);
    if (JS_IsException(fun.get())) {
      return quickjsr::JSValue_to_SEXP(ctx->ctx, fun.get());
    }
    ScopedJSValue result(
      ctx->ctx,
      JS_Call(ctx->ctx, fun.get(), receiver.get(), args.size(), args.data())
    );
    if (JS_IsException(result.get())) {
      return quickjsr::JSValue_to_SEXP(ctx->ctx, result.get());
    }
    ScopedJSValue result_receiver(ctx->ctx, JS_UNDEFINED);
    return make_value_ref(ctx_ptr_, ctx->ctx, result, result_receiver);
    END_CPP11
  }

  SEXP qjs_get_(SEXP ctx_ptr_, SEXP js_obj_name) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);
    ScopedJSValue global(ctx->ctx, JS_GetGlobalObject(ctx->ctx));
    ScopedJSValue result(
      ctx->ctx,
      quickjsr::JS_GetPropertyRecursive(
        ctx->ctx, global.get(), Rf_translateCharUTF8(STRING_ELT(js_obj_name, 0))
      )
    );
    return quickjsr::JSValue_to_SEXP(ctx->ctx, result.get());
    END_CPP11
  }

  SEXP qjs_get_ref_(SEXP ctx_ptr_, SEXP js_obj_name) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);
    ScopedJSValue global(ctx->ctx, JS_GetGlobalObject(ctx->ctx));
    JSValue receiver_value = JS_UNDEFINED;
    ScopedJSValue result(
      ctx->ctx,
      quickjsr::JS_GetPropertyRecursive(
        ctx->ctx, global.get(), Rf_translateCharUTF8(STRING_ELT(js_obj_name, 0)),
        &receiver_value
      )
    );
    ScopedJSValue receiver(ctx->ctx, receiver_value);
    if (JS_IsException(result.get())) {
      return quickjsr::JSValue_to_SEXP(ctx->ctx, result.get());
    }
    return make_value_ref(ctx_ptr_, ctx->ctx, result, receiver);
    END_CPP11
  }

  SEXP qjs_eval_ref_(SEXP ctx_ptr_, SEXP code_) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);
    const char* code = Rf_translateCharUTF8(STRING_ELT(code_, 0));
    ScopedJSValue result(
      ctx->ctx,
      JS_Eval(ctx->ctx, code, strlen(code), "<input>", JS_EVAL_TYPE_GLOBAL)
    );
    if (JS_IsException(result.get())) {
      return quickjsr::JSValue_to_SEXP(ctx->ctx, result.get());
    }
    ScopedJSValue receiver(ctx->ctx, JS_UNDEFINED);
    return make_value_ref(ctx_ptr_, ctx->ctx, result, receiver);
    END_CPP11
  }

  SEXP qjs_value_ref_get_(SEXP ref_ptr_, SEXP name_) {
    BEGIN_CPP11
    quickjsr::JSValueRefData* ref = quickjsr::get_value_ref(ref_ptr_);
    JSValue receiver_value = JS_UNDEFINED;
    ScopedJSValue result(
      ref->ctx,
      quickjsr::JS_GetPropertyRecursive(
        ref->ctx, ref->value, Rf_translateCharUTF8(STRING_ELT(name_, 0)),
        &receiver_value
      )
    );
    ScopedJSValue receiver(ref->ctx, receiver_value);
    if (JS_IsException(result.get())) {
      return quickjsr::JSValue_to_SEXP(ref->ctx, result.get());
    }
    return make_value_ref(R_ExternalPtrProtected(ref_ptr_), ref->ctx, result, receiver);
    END_CPP11
  }

  SEXP qjs_value_ref_call_(SEXP ref_ptr_, SEXP args_list_) {
    BEGIN_CPP11
    quickjsr::JSValueRefData* ref = quickjsr::get_value_ref(ref_ptr_);
    R_xlen_t n_args = Rf_xlength(args_list_);
    ScopedJSValues args(ref->ctx);
    args.values.reserve(static_cast<size_t>(n_args));
    for (R_xlen_t i = 0; i < n_args; i++) {
      args.push(quickjsr::SEXP_to_JSValue(ref->ctx, VECTOR_ELT(args_list_, i), true));
    }
    ScopedJSValue result(
      ref->ctx,
      JS_Call(ref->ctx, ref->value, ref->receiver, args.size(), args.data())
    );
    if (JS_IsException(result.get())) {
      return quickjsr::JSValue_to_SEXP(ref->ctx, result.get());
    }
    ScopedJSValue receiver(ref->ctx, JS_UNDEFINED);
    return make_value_ref(R_ExternalPtrProtected(ref_ptr_), ref->ctx, result, receiver);
    END_CPP11
  }

  SEXP qjs_value_ref_to_r_(SEXP ref_ptr_) {
    BEGIN_CPP11
    quickjsr::JSValueRefData* ref = quickjsr::get_value_ref(ref_ptr_);
    return quickjsr::JSValue_to_SEXP(ref->ctx, ref->value);
    END_CPP11
  }

  SEXP qjs_assign_(SEXP ctx_ptr_, SEXP js_obj_name_, SEXP value_) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);
    ScopedJSValue value(ctx->ctx, quickjsr::SEXP_to_JSValue(ctx->ctx, value_, true));
    ScopedJSValue global(ctx->ctx, JS_GetGlobalObject(ctx->ctx));
    int result = quickjsr::JS_SetPropertyRecursive(
      ctx->ctx, global.get(), Rf_translateCharUTF8(STRING_ELT(js_obj_name_, 0)),
      value.release()
    );
    if (result < 0) {
      return quickjsr::JSValue_to_SEXP(ctx->ctx, JS_EXCEPTION);
    }
    return cpp11::as_sexp(result);
    END_CPP11
  }

  SEXP qjs_eval_(SEXP eval_string_) {
    BEGIN_CPP11
    const char* eval_string = Rf_translateCharUTF8(STRING_ELT(eval_string_, 0));
    JSRuntime* rt = quickjsr::JS_NewCustomRuntime(-1);
    if (!rt) cpp11::stop("Could not create QuickJS runtime");
    ScopedRuntime guard{rt, nullptr, true};
    JSContext* rt_ctx = quickjsr::JS_NewCustomContext(rt);
    guard.ctx = rt_ctx;
    if (!rt_ctx) cpp11::stop("Could not create QuickJS context");

    ScopedJSValue value(
      rt_ctx,
      JS_Eval(rt_ctx, eval_string, strlen(eval_string), "<input>", JS_EVAL_TYPE_GLOBAL)
    );
    return quickjsr::JSValue_to_SEXP(rt_ctx, value.get());
    END_CPP11
  }

  SEXP to_json_(SEXP arg_, SEXP auto_unbox_) {
    BEGIN_CPP11
    JSRuntime* rt = JS_NewRuntime();
    if (!rt) cpp11::stop("Could not create QuickJS runtime");
    ScopedRuntime guard{rt, nullptr, false};
    JSContext* rt_ctx = JS_NewContext(rt);
    guard.ctx = rt_ctx;
    if (!rt_ctx) cpp11::stop("Could not create QuickJS context");

    ScopedJSValue arg(
      rt_ctx,
      quickjsr::SEXP_to_JSValue(rt_ctx, arg_, LOGICAL_ELT(auto_unbox_, 0))
    );
    ScopedJSValue result(
      rt_ctx,
      JS_JSONStringify(rt_ctx, arg.get(), JS_UNDEFINED, JS_UNDEFINED)
    );
    if (JS_IsException(result.get())) {
      return quickjsr::JSValue_to_SEXP(rt_ctx, result.get());
    }
    ScopedCString text(rt_ctx, JS_ToCString(rt_ctx, result.get()));
    if (!text.value) {
      return quickjsr::JSValue_to_SEXP(rt_ctx, JS_EXCEPTION);
    }
    return cpp11::as_sexp(text.value);
    END_CPP11
  }

  SEXP from_json_(SEXP json_) {
    BEGIN_CPP11
    JSRuntime* rt = JS_NewRuntime();
    if (!rt) cpp11::stop("Could not create QuickJS runtime");
    ScopedRuntime guard{rt, nullptr, false};
    JSContext* rt_ctx = JS_NewContext(rt);
    guard.ctx = rt_ctx;
    if (!rt_ctx) cpp11::stop("Could not create QuickJS context");

    const char* json = Rf_translateCharUTF8(STRING_ELT(json_, 0));
    ScopedJSValue result(rt_ctx, JS_ParseJSON(rt_ctx, json, strlen(json), "<input>"));
    return quickjsr::JSValue_to_SEXP(rt_ctx, result.get());
    END_CPP11
  }

  SEXP qjs_version_() {
    BEGIN_CPP11
    return cpp11::as_sexp(JS_GetVersion());
    END_CPP11
  }
}
