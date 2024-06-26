#ifndef QUICKJSR_JSCOMMONTYPE_HPP
#define QUICKJSR_JSCOMMONTYPE_HPP

#include <quickjsr/JSValue_Date.hpp>
#include <quickjs-libc.h>

namespace quickjsr {

enum JSCommonType {
  Integer,
  Double,
  Logical,
  Character,
  Date,
  NumberArray,
  Object,
  Undefined,
  Unknown
};

JSCommonType JS_ArrayCommonType(JSContext* ctx, const JSValue& val);

JSCommonType JS_GetCommonType(JSContext* ctx, const JSValue& val) {
  if (JS_IsUndefined(val)) {
    return Undefined;
  }
  if (JS_IsBool(val)) {
    return Logical;
  }
  if (JS_VALUE_GET_TAG(val) == JS_TAG_INT) {
    return Integer;
  }
  if (JS_IsNumber(val)) {
    return Double;
  }
  if (JS_IsString(val)) {
    return Character;
  }
  if (JS_IsDate(ctx, val)) {
    return Date;
  }
  if (JS_IsArray(ctx, val)) {
    JSCommonType common_type = JS_ArrayCommonType(ctx, val);
    if (common_type == Integer || common_type == Double) {
      return NumberArray;
    }
  }
  if (JS_IsObject(val)) {
    return Object;
  }
  return Unknown;
}

JSCommonType JS_UpdateCommonType(JSCommonType current, JSContext* ctx, const JSValue& val) {
  if (current == Object || current == Unknown) {
    return current;
  }

  JSCommonType new_type = JS_GetCommonType(ctx, val);
  if (current == new_type) {
    return current;
  }
  if (new_type == Undefined) {
    return current;
  }
  // If one, but not both, types are NumberArray or Date (checked above), return Object
  if (new_type == NumberArray || current == NumberArray || new_type == Object
      || new_type == Date || current == Date) {
    return Object;
  }

  switch (new_type) {
    case Integer: {
      switch (current) {
        case Double:
          return Double;
        case Logical:
          return Integer;
        case Character:
          return Character;
        default:
          return Unknown;
      }
    }
    case Double: {
      switch (current) {
        case Integer:
          return Double;
        case Logical:
          return Double;
        case Character:
          return Character;
        default:
          return Unknown;
      }
    }
    case Logical:
      return current;
    case Character:
      return Character;
    default:
      return Unknown;
  }
}

JSCommonType JS_ArrayCommonType(JSContext* ctx, const JSValue& val) {
  JSValue elem = JS_GetPropertyInt64(ctx, val, 0);
  JSCommonType common_type = JS_GetCommonType(ctx, elem);
  JS_FreeValue(ctx, elem);
  if (common_type == Unknown || common_type == Object) {
    return common_type;
  }

  int64_t len;
  JS_GetLength(ctx, val, &len);

  for (int64_t i = 1; i < len; i++) {
    elem = JS_GetPropertyInt64(ctx, val, i);
    common_type = JS_UpdateCommonType(common_type, ctx, elem);
    JS_FreeValue(ctx, elem);

    if (common_type == Unknown || common_type == Object) {
      return common_type;
    }
  }
  return common_type;
}

} // namespace quickjsr

#endif
