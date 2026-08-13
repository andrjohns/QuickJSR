#include <cpp11.hpp>
#include <cpp11/declarations.hpp>
#include <quickjs-libc.h>
#include <quickjs_helpers.hpp>
#include <quickjsr.hpp>

// Bundles the runtime and its context so a single external pointer owns both,
// guaranteeing the context is always freed before the runtime it depends on,
// regardless of R's garbage collection order.
struct QjsRuntimeContext {
  JSContext* ctx;
  JSRuntime* rt;
};

void JS_FreeRuntimeContext(QjsRuntimeContext* handle) {
  JS_FreeContext(handle->ctx);
  js_std_free_handlers(handle->rt);
  JS_FreeRuntime(handle->rt);
  delete handle;
}

struct ScopedRuntime {
  JSRuntime* rt;
  JSContext* ctx;
  ~ScopedRuntime() {
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
  }
};

// Register the cpp11 external pointer type with the correct cleanup/finaliser function
using ContextXPtr = cpp11::external_pointer<QjsRuntimeContext, JS_FreeRuntimeContext>;

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
    JSContext* ctx = quickjsr::JS_NewCustomContext(rt);
    ContextXPtr handle(new QjsRuntimeContext{ctx, rt});

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
    JSValue val = JS_Eval(ctx->ctx, code_string, strlen(code_string), "<input>", JS_EVAL_TYPE_GLOBAL);
    cpp11::sexp rtn = cpp11::as_sexp(!JS_IsException(val));
    JS_FreeValue(ctx->ctx, val);
    return rtn;
    END_CPP11
  }

  SEXP qjs_call_(SEXP ctx_ptr_, SEXP fun_name_, SEXP args_list_) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);

    int64_t n_args = Rf_xlength(args_list_);
    std::vector<JSValue> args(n_args);
    for (int64_t i = 0; i < n_args; i++) {
      args[i] = quickjsr::SEXP_to_JSValue(ctx->ctx, VECTOR_ELT(args_list_, i), true);
    }

    JSValue global = JS_GetGlobalObject(ctx->ctx);
    JSValue fun = quickjsr::JS_GetPropertyRecursive(ctx->ctx, global, Rf_translateCharUTF8(STRING_ELT(fun_name_, 0)));
    JSValue result_js = JS_Call(ctx->ctx, fun, global, args.size(), args.data());

    for (auto&& arg : args) {
      JS_FreeValue(ctx->ctx, arg);
    }
    JS_FreeValue(ctx->ctx, fun);
    JS_FreeValue(ctx->ctx, global);

    cpp11::sexp result = quickjsr::JSValue_to_SEXP(ctx->ctx, result_js);
    JS_FreeValue(ctx->ctx, result_js);
    return result;
    END_CPP11
  }

  SEXP qjs_get_(SEXP ctx_ptr_, SEXP js_obj_name) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);
    JSValue global = JS_GetGlobalObject(ctx->ctx);
    JSValue result = quickjsr::JS_GetPropertyRecursive(ctx->ctx, global, Rf_translateCharUTF8(STRING_ELT(js_obj_name, 0)));
    JS_FreeValue(ctx->ctx, global);
    cpp11::sexp rtn = quickjsr::JSValue_to_SEXP(ctx->ctx, result);
    JS_FreeValue(ctx->ctx, result);
    return rtn;
    END_CPP11
  }

  SEXP qjs_assign_(SEXP ctx_ptr_, SEXP js_obj_name_, SEXP value_) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);
    JSValue value = quickjsr::SEXP_to_JSValue(ctx->ctx, value_, true);
    JSValue global = JS_GetGlobalObject(ctx->ctx);
    int result = quickjsr::JS_SetPropertyRecursive(ctx->ctx, global, Rf_translateCharUTF8(STRING_ELT(js_obj_name_, 0)), value);

    JS_FreeValue(ctx->ctx, global);

    return cpp11::as_sexp(result);
    END_CPP11
  }

  SEXP qjs_eval_(SEXP eval_string_) {
    BEGIN_CPP11
    const char* eval_string = Rf_translateCharUTF8(STRING_ELT(eval_string_, 0));
    JSRuntime* rt = quickjsr::JS_NewCustomRuntime(-1);
    JSContext* rt_ctx = quickjsr::JS_NewCustomContext(rt);
    ScopedRuntime guard{rt, rt_ctx};

    JSValue val = JS_Eval(rt_ctx, eval_string, strlen(eval_string), "<input>", JS_EVAL_TYPE_GLOBAL);
    cpp11::sexp rtn = quickjsr::JSValue_to_SEXP(rt_ctx, val);
    JS_FreeValue(rt_ctx, val);

    return rtn;
    END_CPP11
  }

  SEXP to_json_(SEXP arg_, SEXP auto_unbox_) {
    BEGIN_CPP11
    JSRuntime* rt = JS_NewRuntime();
    JSContext* rt_ctx = JS_NewContext(rt);
    ScopedRuntime guard{rt, rt_ctx};

    JSValue arg = quickjsr::SEXP_to_JSValue(rt_ctx, arg_, LOGICAL_ELT(auto_unbox_, 0));
    JSValue result_js = JS_JSONStringify(rt_ctx, arg, JS_UNDEFINED, JS_UNDEFINED);
    const char* res_str = JS_ToCString(rt_ctx, result_js);
    cpp11::sexp json = cpp11::as_sexp(res_str ? res_str : "");

    JS_FreeCString(rt_ctx, res_str);
    JS_FreeValue(rt_ctx, result_js);
    JS_FreeValue(rt_ctx, arg);

    return json;
    END_CPP11
  }

  SEXP from_json_(SEXP json_) {
    BEGIN_CPP11
    JSRuntime* rt = JS_NewRuntime();
    JSContext* rt_ctx = JS_NewContext(rt);
    ScopedRuntime guard{rt, rt_ctx};

    const char* json = Rf_translateCharUTF8(STRING_ELT(json_, 0));
    JSValue result = JS_ParseJSON(rt_ctx, json, strlen(json), "<input>");
    cpp11::sexp rtn = quickjsr::JSValue_to_SEXP(rt_ctx, result);

    JS_FreeValue(rt_ctx, result);

    return rtn;
    END_CPP11
  }

  SEXP qjs_version_() {
    BEGIN_CPP11
    return cpp11::as_sexp(JS_GetVersion());
    END_CPP11
  }
}
