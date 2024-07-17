#include <cpp11.hpp>
#include <cpp11/declarations.hpp>
#include <quickjs-libc.h>
#include <quickjs_helpers.hpp>
#include <quickjsr.hpp>

using quickjsr::JS_RtCtxContainer;
using RtCtxXPtr = cpp11::external_pointer<JS_RtCtxContainer>;

extern "C" {
  SEXP qjs_context_(SEXP stack_size_) {
    BEGIN_CPP11
    int stack_size = Rf_isInteger(stack_size_) ? INTEGER_ELT(stack_size_, 0)
                                               : static_cast<int>(REAL_ELT(stack_size_, 0));

    RtCtxXPtr rt(new JS_RtCtxContainer(stack_size));

    return cpp11::as_sexp(rt);
    END_CPP11
  }

  SEXP qjs_source_(SEXP ctx_ptr_, SEXP input_, SEXP is_file_) {
    BEGIN_CPP11
    RtCtxXPtr rt_ctx(ctx_ptr_);
    int ret;
    const char* input = Rf_translateCharUTF8(STRING_ELT(input_, 0));
    if (LOGICAL_ELT(is_file_, 0)) {
      ret = quickjsr::eval_file(rt_ctx->ctx, input, -1);
    } else {
      ret = quickjsr::eval_buf(rt_ctx->ctx, input, strlen(input), "<input>", JS_EVAL_TYPE_GLOBAL);
    }
    return cpp11::as_sexp(!ret);
    END_CPP11
  }

  SEXP qjs_validate_(SEXP ctx_ptr_, SEXP code_string_) {
    BEGIN_CPP11
    RtCtxXPtr rt_ctx(ctx_ptr_);
    const char* code_string = Rf_translateCharUTF8(STRING_ELT(code_string_, 0));
    JSValue val = JS_Eval(rt_ctx->ctx, code_string, strlen(code_string), "", JS_EVAL_FLAG_COMPILE_ONLY);
    bool failed = JS_IsException(val);
    JS_FreeValue(rt_ctx->ctx, val);
    return cpp11::as_sexp(!failed);
    END_CPP11
  }

  SEXP qjs_call_(SEXP ctx_ptr_, SEXP fun_name_, SEXP args_list_) {
    BEGIN_CPP11
    RtCtxXPtr rt_ctx(ctx_ptr_);

    int64_t n_args = Rf_xlength(args_list_);
    std::vector<JSValue> args(n_args);
    for (int64_t i = 0; i < n_args; i++) {
      args[i] = quickjsr::SEXP_to_JSValue(rt_ctx->ctx, VECTOR_ELT(args_list_, i), true);
    }

    JSValue global = JS_GetGlobalObject(rt_ctx->ctx);
    JSValue fun = quickjsr::JS_GetPropertyRecursive(rt_ctx->ctx, global, Rf_translateCharUTF8(STRING_ELT(fun_name_, 0)));
    JSValue result_js = JS_Call(rt_ctx->ctx, fun, global, args.size(), args.data());

    SEXP result = quickjsr::JSValue_to_SEXP(rt_ctx->ctx, result_js);

    JS_FreeValue(rt_ctx->ctx, result_js);
    for (auto&& arg : args) {
      JS_FreeValue(rt_ctx->ctx, arg);
    }
    JS_FreeValue(rt_ctx->ctx, fun);
    JS_FreeValue(rt_ctx->ctx, global);

    return result;
    END_CPP11
  }

  SEXP qjs_get_(SEXP ctx_ptr_, SEXP js_obj_name) {
    BEGIN_CPP11
    RtCtxXPtr rt_ctx(ctx_ptr_);
    JSValue global = JS_GetGlobalObject(rt_ctx->ctx);
    JSValue result = quickjsr::JS_GetPropertyRecursive(rt_ctx->ctx, global, Rf_translateCharUTF8(STRING_ELT(js_obj_name, 0)));
    SEXP rtn = quickjsr::JSValue_to_SEXP(rt_ctx->ctx, result);

    JS_FreeValue(rt_ctx->ctx, result);
    JS_FreeValue(rt_ctx->ctx, global);

    return rtn;
    END_CPP11
  }

  SEXP qjs_assign_(SEXP ctx_ptr_, SEXP js_obj_name_, SEXP value_) {
    BEGIN_CPP11
    RtCtxXPtr rt_ctx(ctx_ptr_);
    JSValue global = JS_GetGlobalObject(rt_ctx->ctx);
    JSValue value = quickjsr::SEXP_to_JSValue(rt_ctx->ctx, value_, true);
    int result = quickjsr::JS_SetPropertyRecursive(rt_ctx->ctx, global, Rf_translateCharUTF8(STRING_ELT(js_obj_name_, 0)), value);
    JS_FreeValue(rt_ctx->ctx, global);

    return cpp11::as_sexp(result);
    END_CPP11
  }

  SEXP qjs_eval_(SEXP eval_string_) {
    BEGIN_CPP11
    const char* eval_string = Rf_translateCharUTF8(STRING_ELT(eval_string_, 0));
    JS_RtCtxContainer rt_ctx;

    JSValue val = JS_Eval(rt_ctx.ctx, eval_string, strlen(eval_string), "<input>", JS_EVAL_TYPE_GLOBAL);
    SEXP result = quickjsr::JSValue_to_SEXP(rt_ctx.ctx, val);

    JS_FreeValue(rt_ctx.ctx, val);

    return result;
    END_CPP11
  }

  SEXP to_json_(SEXP arg_, SEXP auto_unbox_) {
    BEGIN_CPP11
    JS_RtCtxContainer rt_ctx;

    JSValue arg = quickjsr::SEXP_to_JSValue(rt_ctx.ctx, arg_, LOGICAL_ELT(auto_unbox_, 0));
    std::string result = quickjsr::JSValue_to_JSON(rt_ctx.ctx, arg);

    JS_FreeValue(rt_ctx.ctx, arg);

    return cpp11::as_sexp(result);
    END_CPP11
  }

  SEXP from_json_(SEXP json_) {
    BEGIN_CPP11
    JS_RtCtxContainer rt_ctx;

    const char* json = Rf_translateCharUTF8(STRING_ELT(json_, 0));
    JSValue result = JS_ParseJSON(rt_ctx.ctx, json, strlen(json), "<input>");
    SEXP rtn = quickjsr::JSValue_to_SEXP(rt_ctx.ctx, result);

    JS_FreeValue(rt_ctx.ctx, result);

    return rtn;
    END_CPP11
  }

  SEXP qjs_version_() {
    BEGIN_CPP11
    return cpp11::as_sexp(JS_GetVersion());
    END_CPP11
  }
}
