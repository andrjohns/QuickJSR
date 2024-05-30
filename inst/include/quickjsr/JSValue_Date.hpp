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

inline JSValue JS_NewDate(JSContext* ctx, double timestamp, bool posixct = false) {
  static constexpr double milliseconds_day = 86400000;
  static constexpr double milliseconds_second = 1000;
  double tz_offset_seconds = get_tz_offset_seconds();
  JSValue global_obj = JS_GetGlobalObject(ctx);
  JSValue date_ctor = JS_GetPropertyStr(ctx, global_obj, "Date");
  JSValue timestamp_val;
  if (posixct) {
    timestamp_val = JS_NewFloat64(ctx, (timestamp + tz_offset_seconds) * milliseconds_second);
  } else {
    timestamp_val = JS_NewFloat64(ctx, timestamp * milliseconds_day);
  }
  JSValue date = JS_CallConstructor(ctx, date_ctor, 1, &timestamp_val);

  JS_FreeValue(ctx, global_obj);
  JS_FreeValue(ctx, date_ctor);
  JS_FreeValue(ctx, timestamp_val);
  return date;
}

inline JSValue JS_GetTime(JSContext* ctx, const JSValue& val) {
  JSAtom timeAtom = JS_NewAtom(ctx, "getTime");
  JSValue timeVal = JS_Invoke(ctx, val, timeAtom, 0, NULL);

  JS_FreeAtom(ctx, timeAtom);

  return timeVal;
}

} // namespace quickjsr

#endif
