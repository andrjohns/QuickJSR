#include <cpp11.hpp>
#include <cpp11/declarations.hpp>
#include <quickjs-libc.h>
#include <quickjsr.hpp>
#include <iostream>

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
  // Workaround for RStan stack overflow until they update
  if (stack_size != -1) {
    JS_SetMaxStackSize(rt.get(), 0);
  }
  js_std_init_handlers(rt.get());

  // Initialise a class which can be used for passing SEXP objects to JS
  // without needing conversion
  JS_NewClass(rt.get(), quickjsr::js_sexp_class_id, &quickjsr::js_sexp_class_def);

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

JSValue get_property_recursive(JSContext* ctx, JSValue obj, const char* name) {
  const char* dot = strchr(name, '.');
  if (dot) {
    // The name contains a ".", so we extract the first property and recurse on the rest of the name
    std::string first_property_name(name, dot - name);
    JSValue first_property = JS_GetPropertyStr(ctx, obj, first_property_name.c_str());
    JSValue result = get_property_recursive(ctx, first_property, dot + 1);
    JS_FreeValue(ctx, first_property);
    return result;
  } else {
    // The name does not contain a ".", so we get the property from the object
    return JS_GetPropertyStr(ctx, obj, name);
  }
}

extern "C" SEXP qjs_call_(SEXP ctx_ptr_, SEXP fun_name_, SEXP args_list_) {
  BEGIN_CPP11
  JSContext* ctx = ContextXPtr(ctx_ptr_).get();

  int n_args = Rf_length(args_list_);
  std::vector<JSValue> args(n_args);
  for (int i = 0; i < n_args; i++) {
    args[i] = quickjsr::SEXP_to_JSValue(ctx, VECTOR_ELT(args_list_, i), true);
  }

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue fun = get_property_recursive(ctx, global, CHAR(STRING_ELT(fun_name_, 0)));
  JSValue result_js = JS_Call(ctx, fun, global, args.size(), args.data());

  SEXP result;
  if (JS_IsException(result_js)) {
    js_std_dump_error(ctx);
    result =  cpp11::as_sexp("Error!");
  } else {
    result = quickjsr::JSValue_to_SEXP(ctx, result_js);
  }

  JS_FreeValue(ctx, result_js);
  for (auto&& arg : args) {
    JS_FreeValue(ctx, arg);
  }
  JS_FreeValue(ctx, fun);
  JS_FreeValue(ctx, global);

  return result;
  END_CPP11
}

extern "C" SEXP qjs_eval_(SEXP eval_string_) {
  BEGIN_CPP11
  std::string eval_string = cpp11::as_cpp<std::string>(eval_string_);
  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  JSValue val = JS_Eval(ctx, eval_string.c_str(), eval_string.size(), "", 0);
  SEXP result;
  if (JS_IsException(val)) {
    js_std_dump_error(ctx);
    result =  cpp11::as_sexp("Error!");
  } else {
    result = quickjsr::JSValue_to_SEXP(ctx, val);
  }

  JS_FreeValue(ctx, val);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  return result;
  END_CPP11
}

extern "C" SEXP to_json_(SEXP arg_, SEXP auto_unbox_) {
  BEGIN_CPP11
  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  JSValue arg = quickjsr::SEXP_to_JSValue(ctx, arg_,
                                          cpp11::as_cpp<bool>(auto_unbox_));
  std::string result = quickjsr::JSValue_to_JSON(ctx, arg);

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
