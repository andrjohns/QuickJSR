#include <cpp11.hpp>
#include <cpp11/declarations.hpp>
#include <fstream>
#include <sstream>

using JSvalue = struct JSValue;
using JSRuntime = struct JSRuntime;
using JSContext = struct JSContext;

// We compile the functions as a separate C translation unit, as the QuickJS
// C headers trigger -Wpedantic warnings under C++
#ifdef __cplusplus
extern "C" {
#endif

  JSRuntime* JS_NewRuntime(void);
  void JS_SetMaxStackSize(JSRuntime* rt, size_t stack_size);
  JSContext* JS_NewContext(JSRuntime* rt);
  void JS_FreeContext(JSContext* ctx);
  void JS_FreeRuntime(JSRuntime* rt);
  void js_std_init_handlers(JSRuntime *rt);
  void js_std_add_helpers(JSContext *ctx, int argc, char **argv);

  bool qjs_source_impl(JSContext* ctx, const char* code_string);
  bool qjs_validate_impl(JSContext* ctx, const char* function_name);
  const char* qjs_call_impl(JSContext* ctx, const char* wrapped_name,
                        const char* call_wrapper, const char* args_json);
  const char* qjs_eval_impl(const char* eval_string);

#ifdef __cplusplus
}
#endif

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
  return cpp11::as_sexp(qjs_source_impl(ctx, code_string.c_str()));
  END_CPP11
}

extern "C" SEXP qjs_validate_(SEXP ctx_ptr_, SEXP code_string_) {
  BEGIN_CPP11
  JSContext* ctx = ContextXPtr(ctx_ptr_).get();
  std::string code_string = cpp11::as_cpp<std::string>(code_string_);
  return cpp11::as_sexp(qjs_validate_impl(ctx, code_string.c_str()));
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

  return cpp11::as_sexp(qjs_call_impl(ctx, wrapped_name.c_str(), call_wrapper.c_str(),
                                      args_json.c_str()));
  END_CPP11
}

extern "C" SEXP qjs_eval_(SEXP eval_string_) {
  BEGIN_CPP11
  std::string eval_string = cpp11::as_cpp<std::string>(eval_string_);
  return cpp11::as_sexp(qjs_eval_impl(eval_string.c_str()));
  END_CPP11
}
