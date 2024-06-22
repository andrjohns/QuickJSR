#ifndef QUICKJSR_JSVALUE_DATE_HPP
#define QUICKJSR_JSVALUE_DATE_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <cstring>

namespace quickjsr {

inline double get_tz_offset_seconds() {
  cpp11::function get_tz_offset_seconds = cpp11::package("QuickJSR")["get_tz_offset_seconds"];
  return cpp11::as_cpp<double>(get_tz_offset_seconds());
}

inline bool JS_IsDate(JSContext* ctx, const JSValue& val) {
  JSValue ctor = JS_GetPropertyStr(ctx, val, "constructor");
  if (JS_IsException(ctor)) {
    JS_FreeValue(ctx, ctor);
    return false;
  }
  JSValue ctorName = JS_GetPropertyStr(ctx, ctor, "name");

  const char* name = JS_ToCString(ctx, ctorName);
  bool isDate = strcmp(name, "Date") == 0;

  JS_FreeCString(ctx, name);
  JS_FreeValue(ctx, ctorName);
  JS_FreeValue(ctx, ctor);

  return isDate;
}

inline JSValue JS_GetTime(JSContext* ctx, const JSValue& val) {
  JSAtom timeAtom = JS_NewAtom(ctx, "getTime");
  JSValue timeVal = JS_Invoke(ctx, val, timeAtom, 0, NULL);

  JS_FreeAtom(ctx, timeAtom);

  return timeVal;
}

} // namespace quickjsr

#endif
