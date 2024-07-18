#include <cpp11.hpp>
#include <cpp11/declarations.hpp>
#include <quickjs-libc.h>
#include <quickjs_helpers.hpp>
#include <quickjsr.hpp>

using quickjsr::JS_RtCtxContainer;
using quickjsr::JS_ValContainer;
using quickjsr::RtCtxXPtr;
using quickjsr::to_cstring;

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
    const char* input = to_cstring(input_);
    if (LOGICAL_ELT(is_file_, 0)) {
      ret = quickjsr::eval_file(rt_ctx, input, -1);
    } else {
      ret = quickjsr::eval_buf(rt_ctx, input, strlen(input), "<input>", JS_EVAL_TYPE_GLOBAL);
    }
    return cpp11::as_sexp(!ret);
    END_CPP11
  }

  SEXP qjs_validate_(SEXP ctx_ptr_, SEXP code_string_) {
    BEGIN_CPP11
    RtCtxXPtr rt_ctx(ctx_ptr_);
    const char* code_string = to_cstring(code_string_);
    JS_ValContainer val(rt_ctx, JS_Eval(rt_ctx, code_string, strlen(code_string), "", JS_EVAL_FLAG_COMPILE_ONLY));
    return cpp11::as_sexp(!JS_IsException(val));
    END_CPP11
  }

  SEXP qjs_call_(SEXP ctx_ptr_, SEXP fun_name_, SEXP args_list_) {
    BEGIN_CPP11
    RtCtxXPtr rt_ctx(ctx_ptr_);

    int64_t n_args = Rf_xlength(args_list_);
    std::vector<JSValue> args(n_args);
    for (int64_t i = 0; i < n_args; i++) {
      args[i] = quickjsr::SEXP_to_JSValue(rt_ctx, VECTOR_ELT(args_list_, i), true);
    }

    JS_ValContainer global(rt_ctx, JS_GetGlobalObject(rt_ctx));
    JS_ValContainer fun(rt_ctx, quickjsr::JS_GetPropertyRecursive(rt_ctx, global, to_cstring(fun_name_)));
    JS_ValContainer result_js(rt_ctx, JS_Call(rt_ctx, fun, global, args.size(), args.data()));

    for (auto&& arg : args) {
      JS_FreeValue(rt_ctx, arg);
    }

    return quickjsr::JSValue_to_SEXP(rt_ctx, result_js);
    END_CPP11
  }

  SEXP qjs_get_(SEXP ctx_ptr_, SEXP js_obj_name) {
    BEGIN_CPP11
    RtCtxXPtr rt_ctx(ctx_ptr_);
    JS_ValContainer global(rt_ctx, JS_GetGlobalObject(rt_ctx));
    JS_ValContainer result(rt_ctx, quickjsr::JS_GetPropertyRecursive(rt_ctx, global, to_cstring(js_obj_name)));
    return quickjsr::JSValue_to_SEXP(rt_ctx, result);
    END_CPP11
  }

  SEXP qjs_assign_(SEXP ctx_ptr_, SEXP js_obj_name_, SEXP value_) {
    BEGIN_CPP11
    RtCtxXPtr rt_ctx(ctx_ptr_);
    JS_ValContainer global(rt_ctx, JS_GetGlobalObject(rt_ctx));
    JS_ValContainer value(rt_ctx, quickjsr::SEXP_to_JSValue(rt_ctx, value_, true));
    int result = quickjsr::JS_SetPropertyRecursive(rt_ctx, global, to_cstring(js_obj_name_), value);

    return cpp11::as_sexp(result);
    END_CPP11
  }

  SEXP qjs_eval_(SEXP eval_string_) {
    BEGIN_CPP11
    const char* eval_string = to_cstring(eval_string_);
    RtCtxXPtr rt_ctx(new JS_RtCtxContainer());
    JS_ValContainer val(rt_ctx, JS_Eval(rt_ctx, eval_string, strlen(eval_string), "<input>", JS_EVAL_TYPE_GLOBAL));
    return quickjsr::JSValue_to_SEXP(rt_ctx, val);
    END_CPP11
  }

  SEXP to_json_(SEXP arg_, SEXP auto_unbox_) {
    BEGIN_CPP11
    RtCtxXPtr rt_ctx(new JS_RtCtxContainer());
    JS_ValContainer arg(rt_ctx, quickjsr::SEXP_to_JSValue(rt_ctx, arg_, LOGICAL_ELT(auto_unbox_, 0)));
    return cpp11::as_sexp(quickjsr::JSValue_to_JSON(rt_ctx, arg));
    END_CPP11
  }

  SEXP from_json_(SEXP json_) {
    BEGIN_CPP11
    RtCtxXPtr rt_ctx(new JS_RtCtxContainer());

    const char* json = to_cstring(json_);
    JS_ValContainer result(rt_ctx, JS_ParseJSON(rt_ctx, json, strlen(json), "<input>"));
    return quickjsr::JSValue_to_SEXP(rt_ctx, result);
    END_CPP11
  }

  SEXP qjs_version_() {
    BEGIN_CPP11
    return cpp11::as_sexp(JS_GetVersion());
    END_CPP11
  }
}
