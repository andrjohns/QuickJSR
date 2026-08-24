#ifndef QUICKJSR_JS_GET_PROPERTY_RECURSIVE_HPP
#define QUICKJSR_JS_GET_PROPERTY_RECURSIVE_HPP

#include <quickjs-libc.h>
#include <string_view>

namespace quickjsr {
  inline JSValue JS_GetPropertyRecursive(JSContext* ctx, JSValue obj,
                                         std::string_view name,
                                         JSValue* receiver = nullptr) {
    const size_t dot = name.find('.');
    const size_t length = dot == std::string_view::npos ? name.size() : dot;
    JSAtom atom = JS_NewAtomLen(ctx, name.data(), length);
    if (atom == JS_ATOM_NULL) {
      return JS_EXCEPTION;
    }
    if (dot == std::string_view::npos && receiver) {
      *receiver = JS_DupValue(ctx, obj);
    }
    JSValue property = JS_GetProperty(ctx, obj, atom);
    JS_FreeAtom(ctx, atom);
    if (dot == std::string_view::npos || JS_IsException(property)) {
      return property;
    }
    JSValue result = JS_GetPropertyRecursive(
      ctx, property, name.substr(dot + 1), receiver
    );
    JS_FreeValue(ctx, property);
    return result;
  }

  inline int JS_SetPropertyRecursive(JSContext* ctx, JSValue obj,
                                     std::string_view name, JSValue value) {
    const size_t dot = name.find('.');
    const size_t length = dot == std::string_view::npos ? name.size() : dot;
    JSAtom atom = JS_NewAtomLen(ctx, name.data(), length);
    if (atom == JS_ATOM_NULL) {
      JS_FreeValue(ctx, value);
      return -1;
    }
    if (dot != std::string_view::npos) {
      JSValue property = JS_GetProperty(ctx, obj, atom);
      JS_FreeAtom(ctx, atom);
      if (JS_IsException(property)) {
        JS_FreeValue(ctx, value);
        return -1;
      }
      int result = JS_SetPropertyRecursive(
        ctx, property, name.substr(dot + 1), value
      );
      JS_FreeValue(ctx, property);
      return result;
    }
    int result = JS_SetProperty(ctx, obj, atom, value);
    JS_FreeAtom(ctx, atom);
    return result;
  }
}

#endif
