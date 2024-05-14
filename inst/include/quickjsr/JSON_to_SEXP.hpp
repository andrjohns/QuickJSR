#ifndef QUICKJSR_JSON_TO_SEXP_HPP
#define QUICKJSR_JSON_TO_SEXP_HPP

#include <quickjsr/JSValue_to_SEXP.hpp>
#include <cpp11.hpp>
#include <quickjs-libc.h>

namespace quickjsr {

SEXP JSON_to_SEXP(JSContext* ctx, const std::string& json) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue json_obj = JS_GetPropertyStr(ctx, global, "JSON");
  JSValue parse = JS_GetPropertyStr(ctx, json_obj, "parse");

  JSValue json_str = JS_NewString(ctx, json.c_str());
  JSValue result = JS_Call(ctx, parse, global, 1, &json_str);

  SEXP rtn;
  if (JS_IsException(result)) {
    js_std_dump_error(ctx);
    rtn = cpp11::as_sexp("Error!");
  } else {
    rtn = JSValue_to_SEXP(ctx, result);
  }

  JS_FreeValue(ctx, result);
  JS_FreeValue(ctx, json_str);
  JS_FreeValue(ctx, parse);
  JS_FreeValue(ctx, json_obj);
  JS_FreeValue(ctx, global);

  return rtn;
}

} // namespace quickjsr
#endif
