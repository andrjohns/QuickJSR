#include <cpp11.hpp>
#include <cpp11/declarations.hpp>
#include <quickjs-libc.h>
#include <quickjsr.hpp>

void JS_FreeRuntimeStdHandlers(JSRuntime* rt) {
  js_std_free_handlers(rt);
  JS_FreeRuntime(rt);
}

// Register the cpp11 external pointer types with the correct cleanup/finaliser functions
using ContextXPtr = cpp11::external_pointer<JSContext, JS_FreeContext>;
using RuntimeXPtr = cpp11::external_pointer<JSRuntime, JS_FreeRuntimeStdHandlers>;

extern "C" SEXP qjs_context_(SEXP stack_size_) {
  BEGIN_CPP11
  int stack_size = cpp11::as_cpp<int>(stack_size_);

  RuntimeXPtr rt(JS_NewRuntime());
  if (stack_size != -1) {
    JS_SetMaxStackSize(rt.get(), stack_size);
  }
  js_std_init_handlers(rt.get());
  ContextXPtr ctx(JS_NewContext(rt.get()));
  js_std_add_helpers(ctx.get(), 0, (char**)"");

  cpp11::writable::list result;
  using cpp11::literals::operator""_nm;
  result.push_back({"runtime_ptr"_nm = rt});
  result.push_back({"context_ptr"_nm = ctx});

  return cpp11::as_sexp(result);
  END_CPP11
}

extern "C" SEXP qjs_source_(SEXP ctx_ptr_, SEXP code_string_) {
  BEGIN_CPP11
  JSContext* ctx = ContextXPtr(ctx_ptr_).get();
  std::string code_string = cpp11::as_cpp<std::string>(code_string_);
  JSValue val = JS_Eval(ctx, code_string.c_str(), code_string.size(), "", 0);
  bool failed = JS_IsException(val);
  if (failed) {
    js_std_dump_error(ctx);
  }
  JS_FreeValue(ctx, val);
  return cpp11::as_sexp(!failed);
  END_CPP11
}

extern "C" SEXP qjs_validate_(SEXP ctx_ptr_, SEXP code_string_) {
  BEGIN_CPP11
  JSContext* ctx = ContextXPtr(ctx_ptr_).get();
  std::string code_string = cpp11::as_cpp<std::string>(code_string_);
  JSValue val = JS_Eval(ctx, code_string.c_str(), code_string.size(), "", JS_EVAL_FLAG_COMPILE_ONLY);
  bool failed = JS_IsException(val);
  JS_FreeValue(ctx, val);
  return cpp11::as_sexp(!failed);
  END_CPP11
}

extern "C" SEXP qjs_call_(SEXP ctx_ptr_, SEXP function_name_, SEXP args_json_) {
  BEGIN_CPP11
  JSContext* ctx = ContextXPtr(ctx_ptr_).get();
  std::string function_name = cpp11::as_cpp<std::string>(function_name_);
  std::string args_json = cpp11::as_cpp<std::string>(args_json_);
  // Arguments are passed from R as a JSON object string, so we use a wrapper function
  // which 'spreads' the object to separate arguments.
  std::string wrapped_name = function_name + "_wrapper";
  std::string call_wrapper =
    "function " + wrapped_name + "(args_object) { return " + function_name +
    "(...Object.values(JSON.parse(args_object))); }";

  JSValue tmp = JS_Eval(ctx, call_wrapper.c_str(), call_wrapper.size(), "", 0);
  bool failed = JS_IsException(tmp);
  JS_FreeValue(ctx, tmp);
  if (failed) {
    js_std_dump_error(ctx);
    return cpp11::as_sexp("Error!");
  }

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue function_wrapper = JS_GetPropertyStr(ctx, global, wrapped_name.c_str());
  JSValue args[] = {
    JS_NewString(ctx, args_json.c_str())
  };

  JSValue result_js = JS_Call(ctx, function_wrapper, global, 1, args);
  std::string result;
  if (JS_IsException(result_js)) {
    js_std_dump_error(ctx);
    result = "Error!";
  } else {
    result = quickjsr::JSValue_to_JSON(ctx, &result_js);
  }

  JS_FreeValue(ctx, result_js);
  JS_FreeValue(ctx, args[0]);
  JS_FreeValue(ctx, function_wrapper);
  JS_FreeValue(ctx, global);

  return cpp11::as_sexp(result);
  END_CPP11
}

extern "C" SEXP qjs_eval_(SEXP eval_string_) {
  BEGIN_CPP11
  std::string eval_string = cpp11::as_cpp<std::string>(eval_string_);
  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  JSValue val = JS_Eval(ctx, eval_string.c_str(), eval_string.size(), "", 0);
  std::string result;
  if (JS_IsException(val)) {
    js_std_dump_error(ctx);
    result = "Error!";
  } else {
    result = quickjsr::JSValue_to_JSON(ctx, &val);
  }

  JS_FreeValue(ctx, val);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  return cpp11::as_sexp(result);
  END_CPP11
}

extern "C" SEXP to_json_(SEXP arg_) {
  BEGIN_CPP11
  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  JSValue arg = quickjsr::SEXP_to_JSValue(ctx, arg_);
  std::string result = quickjsr::JSValue_to_JSON(ctx, &arg);

  JS_FreeValue(ctx, arg);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  return cpp11::as_sexp(result);
  END_CPP11
}

extern "C" SEXP from_json_(SEXP json_) {
  BEGIN_CPP11
  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  std::string json = cpp11::as_cpp<std::string>(json_);
  JSValue result = quickjsr::JSON_to_JSValue(ctx, json);
  SEXP rtn = quickjsr::JSValue_to_SEXP(ctx, result);

  JS_FreeValue(ctx, result);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  return rtn;
  END_CPP11
}
