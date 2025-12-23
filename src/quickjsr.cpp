#include <cpp11.hpp>
#include <cpp11/declarations.hpp>
#include <quickjs-libc.h>
#include <quickjs_helpers.hpp>
#include <quickjsr.hpp>

using quickjsr::JS_NewCustomRuntime;
using quickjsr::JS_NewCustomContext;

void JS_FreeRuntimeStdHandlers(JSRuntime* rt) {
  js_std_free_handlers(rt);
  JS_FreeRuntime(rt);
}

// Register the cpp11 external pointer types with the correct cleanup/finaliser functions
using ContextXPtr = cpp11::external_pointer<JSContext, JS_FreeContext>;
using RuntimeXPtr = cpp11::external_pointer<JSRuntime, JS_FreeRuntimeStdHandlers>;

extern "C" {
  SEXP qjs_context_(SEXP stack_size_) {
    BEGIN_CPP11
    int stack_size = Rf_isInteger(stack_size_) ? INTEGER_ELT(stack_size_, 0)
                                               : static_cast<int>(REAL_ELT(stack_size_, 0));
    RuntimeXPtr rt(quickjsr::JS_NewCustomRuntime(stack_size));
    ContextXPtr ctx(quickjsr::JS_NewCustomContext(rt.get()));

    cpp11::writable::list result;
    using cpp11::literals::operator""_nm;
    result.push_back({"runtime_ptr"_nm = rt});
    result.push_back({"context_ptr"_nm = ctx});

    return cpp11::as_sexp(result);
    END_CPP11
  }

  SEXP qjs_source_(SEXP ctx_ptr_, SEXP input_, SEXP is_file_) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);
    int ret;
    const char* input = Rf_translateCharUTF8(STRING_ELT(input_, 0));
    if (LOGICAL_ELT(is_file_, 0)) {
      ret = quickjsr::eval_file(ctx.get(), input, -1);
    } else {
      ret = quickjsr::eval_buf(ctx.get(), input, strlen(input), "<input>", JS_EVAL_TYPE_GLOBAL);
    }
    return cpp11::as_sexp(!ret);
    END_CPP11
  }

  SEXP qjs_validate_(SEXP ctx_ptr_, SEXP code_string_) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);
    const char* code_string = Rf_translateCharUTF8(STRING_ELT(code_string_, 0));
    JSValue val = JS_Eval(ctx.get(), code_string, strlen(code_string), "<input>", JS_EVAL_TYPE_GLOBAL);
    SEXP rtn = cpp11::as_sexp(!JS_IsException(val));
    PROTECT(rtn);
    JS_FreeValue(ctx.get(), val);
    UNPROTECT(1);
    return rtn;
    END_CPP11
  }

  SEXP qjs_call_(SEXP ctx_ptr_, SEXP fun_name_, SEXP args_list_) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);

    int64_t n_args = Rf_xlength(args_list_);
    std::vector<JSValue> args(n_args);
    for (int64_t i = 0; i < n_args; i++) {
      args[i] = quickjsr::SEXP_to_JSValue(ctx.get(), VECTOR_ELT(args_list_, i), true);
    }

    JSValue global = JS_GetGlobalObject(ctx.get());
    JSValue fun = quickjsr::JS_GetPropertyRecursive(ctx.get(), global, Rf_translateCharUTF8(STRING_ELT(fun_name_, 0)));
    JSValue result_js = JS_Call(ctx.get(), fun, global, args.size(), args.data());

    for (auto&& arg : args) {
      JS_FreeValue(ctx.get(), arg);
    }

    SEXP result = quickjsr::JSValue_to_SEXP(ctx.get(), result_js);
    PROTECT(result);
    JS_FreeValue(ctx.get(), fun);
    JS_FreeValue(ctx.get(), global);
    JS_FreeValue(ctx.get(), result_js);
    UNPROTECT(1);

    return result;
    END_CPP11
  }

  SEXP qjs_get_(SEXP ctx_ptr_, SEXP js_obj_name) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);
    JSValue global = JS_GetGlobalObject(ctx.get());
    JSValue result = quickjsr::JS_GetPropertyRecursive(ctx.get(), global, Rf_translateCharUTF8(STRING_ELT(js_obj_name, 0)));
    SEXP rtn = quickjsr::JSValue_to_SEXP(ctx.get(), result);
    PROTECT(rtn);
    JS_FreeValue(ctx.get(), result);
    JS_FreeValue(ctx.get(), global);
    UNPROTECT(1);
    return rtn;
    END_CPP11
  }

  SEXP qjs_assign_(SEXP ctx_ptr_, SEXP js_obj_name_, SEXP value_) {
    BEGIN_CPP11
    ContextXPtr ctx(ctx_ptr_);
    JSValue global = JS_GetGlobalObject(ctx.get());
    JSValue value = quickjsr::SEXP_to_JSValue(ctx.get(), value_, true);
    int result = quickjsr::JS_SetPropertyRecursive(ctx.get(), global, Rf_translateCharUTF8(STRING_ELT(js_obj_name_, 0)), value);

    JS_FreeValue(ctx.get(), value);
    JS_FreeValue(ctx.get(), global);

    return cpp11::as_sexp(result);
    END_CPP11
  }

  SEXP qjs_eval_(SEXP eval_string_) {
    BEGIN_CPP11
    const char* eval_string = Rf_translateCharUTF8(STRING_ELT(eval_string_, 0));
    JSRuntime* rt = quickjsr::JS_NewCustomRuntime(-1);
    JSContext* rt_ctx = quickjsr::JS_NewCustomContext(rt);

    JSValue val = JS_Eval(rt_ctx, eval_string, strlen(eval_string), "<input>", JS_EVAL_TYPE_GLOBAL);
    SEXP rtn = quickjsr::JSValue_to_SEXP(rt_ctx, val);
    PROTECT(rtn);
    JS_FreeValue(rt_ctx, val);
    JS_FreeContext(rt_ctx);
    JS_FreeRuntime(rt);
    UNPROTECT(1);

    return rtn;
    END_CPP11
  }

  SEXP to_json_(SEXP arg_, SEXP auto_unbox_) {
    BEGIN_CPP11
    JSRuntime* rt = JS_NewRuntime();
    JSContext* rt_ctx = JS_NewContext(rt);

    JSValue arg = quickjsr::SEXP_to_JSValue(rt_ctx, arg_, LOGICAL_ELT(auto_unbox_, 0));
    JSValue result_js = JS_JSONStringify(rt_ctx, arg, JS_UNDEFINED, JS_UNDEFINED);
    const char* res_str = JS_ToCString(rt_ctx, result_js);
    SEXP json = cpp11::as_sexp(res_str ? res_str : "");
    PROTECT(json);

    JS_FreeCString(rt_ctx, res_str);
    JS_FreeValue(rt_ctx, result_js);
    JS_FreeValue(rt_ctx, arg);
    JS_FreeContext(rt_ctx);
    JS_FreeRuntime(rt);
    UNPROTECT(1);

    return json;
    END_CPP11
  }

  SEXP from_json_(SEXP json_) {
    BEGIN_CPP11
    JSRuntime* rt = JS_NewRuntime();
    JSContext* rt_ctx = JS_NewContext(rt);

    const char* json = Rf_translateCharUTF8(STRING_ELT(json_, 0));
    JSValue result = JS_ParseJSON(rt_ctx, json, strlen(json), "<input>");
    SEXP rtn = quickjsr::JSValue_to_SEXP(rt_ctx, result);
    PROTECT(rtn);
    JS_FreeValue(rt_ctx, result);
    JS_FreeContext(rt_ctx);
    JS_FreeRuntime(rt);
    UNPROTECT(1);

    return rtn;
    END_CPP11
  }

  SEXP qjs_version_() {
    BEGIN_CPP11
    return cpp11::as_sexp(JS_GetVersion());
    END_CPP11
  }
}
