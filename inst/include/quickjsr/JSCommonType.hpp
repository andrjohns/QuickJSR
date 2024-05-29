#ifndef QUICKJSR_JSCOMMONTYPE_HPP
#define QUICKJSR_JSCOMMONTYPE_HPP

#include <quickjs-libc.h>

namespace quickjsr {

enum JSCommonType {
  Integer,
  Double,
  Logical,
  Character,
  Object
};

JSCommonType JS_GetCommonType(const JSValue& val) {
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
  return Object;
}

JSCommonType JS_UpdateCommonType(JSCommonType current, const JSValue& val) {
  if (current == Object) {
    return Object;
  }
  switch (JS_GetCommonType(val)) {
    case Integer: {
      switch (current) {
        case Integer:
          return Integer;
        case Double:
          return Double;
        case Logical:
          return Integer;
        case Character:
          return Character;
        default:
          return Object;
      }
    }
    case Double: {
      switch (current) {
        case Integer:
          return Double;
        case Double:
          return Double;
        case Logical:
          return Double;
        case Character:
          return Character;
        default:
          return Object;
      }
    }
    case Logical: {
      switch (current) {
        case Integer:
          return Integer;
        case Double:
          return Double;
        case Logical:
          return Logical;
        case Character:
          return Character;
        default:
          return Object;
      }
    }
    case Character:
      return Character;
    default:
      return Object;
  }
}
} // namespace quickjsr

#endif
