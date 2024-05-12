#include <cpp11.hpp>
#include <cpp11/declarations.hpp>
#include <quickjs-libc.h>

// Register the cpp11 external pointer types with the correct cleanup/finaliser functions
using ContextXPtr = cpp11::external_pointer<JSContext, JS_FreeContext>;
using RuntimeXPtr = cpp11::external_pointer<JSRuntime, JS_FreeRuntime>;

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

const char* JS_ValToJSON(JSContext* ctx, JSValue* val) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue json = JS_GetPropertyStr(ctx, global, "JSON");
  JSValue stringify = JS_GetPropertyStr(ctx, json, "stringify");

  JSValue result_js = JS_Call(ctx, stringify, global, 1, val);
  const char* result;
  if (JS_IsException(result_js)) {
    js_std_dump_error(ctx);
    result = "Error!";
  } else {
    result = JS_ToCString(ctx, result_js);
  }

  JS_FreeValue(ctx, result_js);
  JS_FreeValue(ctx, stringify);
  JS_FreeValue(ctx, json);
  JS_FreeValue(ctx, global);

  return result;
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
  const char* result;
  if (JS_IsException(result_js)) {
    js_std_dump_error(ctx);
    result = "Error!";
  } else {
    result = JS_ValToJSON(ctx, &result_js);
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
  const char* result;
  if (JS_IsException(val)) {
    js_std_dump_error(ctx);
    result = "Error!";
  } else {
    result = JS_ValToJSON(ctx, &val);
  }

  JS_FreeValue(ctx, val);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  return cpp11::as_sexp(result);
  END_CPP11
}

JSValue SEXP_to_JSValue_scalar(JSContext* ctx, SEXP x, int i = 0) {
  switch(TYPEOF(x)) {
    case REALSXP:
      return JS_NewFloat64(ctx, REAL(x)[i]);
    case INTSXP:
      return JS_NewInt32(ctx, INTEGER(x)[i]);
    case LGLSXP:
      return JS_NewBool(ctx, LOGICAL(x)[i]);
    case STRSXP:
      return JS_NewString(ctx, CHAR(STRING_ELT(x, i)));
    default:
      return JS_UNDEFINED;
  }
}

JSValue SEXP_to_JSValue(JSContext* ctx, SEXP x) {
  if (TYPEOF(x) == VECSXP) {
    JSValue obj = JS_NewObject(ctx);
    for (int i = 0; i < Rf_length(x); i++) {
      SEXP name = STRING_ELT(Rf_getAttrib(x, R_NamesSymbol), i);
      JSValue val = SEXP_to_JSValue(ctx, VECTOR_ELT(x, i));
      JS_SetPropertyStr(ctx, obj, CHAR(name), val);
    }
    return obj;
  }
  if (Rf_length(x) == 1) {
    return SEXP_to_JSValue_scalar(ctx, x);
  } else {
    JSValue arr = JS_NewArray(ctx);
    for (int i = 0; i < Rf_length(x); i++) {
      JSValue val = SEXP_to_JSValue_scalar(ctx, x, i);
      JS_SetPropertyUint32(ctx, arr, i, val);
    }
    return arr;
  }
}

SEXP JSValue_to_SEXP_scalar(JSContext* ctx, JSValue val) {
  if (JS_IsBool(val)) {
    return cpp11::as_sexp(static_cast<bool>(JS_ToBool(ctx, val)));
  }
  if (JS_IsNumber(val)) {
    double res;
    JS_ToFloat64(ctx, &res, val);
    return cpp11::as_sexp(res);
  }
  if (JS_IsString(val)) {
    return cpp11::as_sexp(JS_ToCString(ctx, val));
  }
  return cpp11::as_sexp("Unsupported type");
}

SEXP JSValue_to_SEXP(JSContext* ctx, JSValue val) {
  // TODO: Implement array and object conversion
  return JSValue_to_SEXP_scalar(ctx, val);
}

extern "C" SEXP qjs_passthrough_(SEXP args_) {
  BEGIN_CPP11
  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  std::string function_string = "function passthrough(x) { return x; }";
  JSValue tmp = JS_Eval(ctx, function_string.c_str(), function_string.size(), "", 0);
  bool failed = JS_IsException(tmp);
  JS_FreeValue(ctx, tmp);
  if (failed) {
    js_std_dump_error(ctx);
    return cpp11::as_sexp("Error!");
  }
  std::string wrapped_name = "passthrough";
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue function_wrapper = JS_GetPropertyStr(ctx, global, wrapped_name.c_str());
  JSValue args[] = { SEXP_to_JSValue(ctx, args_) };
  JSValue result_js = JS_Call(ctx, function_wrapper, global, 1, args);

  SEXP result;
  if (JS_IsException(result_js)) {
    js_std_dump_error(ctx);
    result = cpp11::as_sexp("Error!");
  } else {
    result = JSValue_to_SEXP(ctx, result_js);
  }

  JS_FreeValue(ctx, result_js);
  JS_FreeValue(ctx, args[0]);
  JS_FreeValue(ctx, function_wrapper);
  JS_FreeValue(ctx, global);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  return cpp11::as_sexp(result);
  END_CPP11
}
