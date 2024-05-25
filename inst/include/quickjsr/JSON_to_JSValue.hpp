#ifndef QUICKJSR_JSON_TO_JSVALUE_HPP
#define QUICKJSR_JSON_TO_JSVALUE_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>

namespace quickjsr {

JSValue JSON_to_JSValue(JSContext* ctx, const std::string& json) {
  JSValue result = JS_ParseJSON(ctx, json.c_str(), json.size(), "<input>");

  if (JS_IsException(result)) {
    js_std_dump_error(ctx);
  }

  return result;
}

} // namespace quickjsr
#endif
