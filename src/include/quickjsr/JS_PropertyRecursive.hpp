#ifndef QUICKJSR_JS_GET_PROPERTY_RECURSIVE_HPP
#define QUICKJSR_JS_GET_PROPERTY_RECURSIVE_HPP

#include <quickjs-libc.h>
#include <string>
#include <cstring>

namespace quickjsr {
  inline JSValue JS_GetPropertyRecursive(JSContext* ctx, JSValue obj, const char* name,
                                         JSValue* receiver = nullptr) {
    const char* dot = strchr(name, '.');
    if (dot) {
      std::string first_property_name(name, dot - name);
      JSValue first_property = JS_GetPropertyStr(ctx, obj, first_property_name.c_str());
      if (JS_IsException(first_property)) {
        return first_property;
      }
      JSValue result = JS_GetPropertyRecursive(ctx, first_property, dot + 1, receiver);
      JS_FreeValue(ctx, first_property);
      return result;
    } else {
      if (receiver) {
        *receiver = JS_DupValue(ctx, obj);
      }
      return JS_GetPropertyStr(ctx, obj, name);
    }
  }

  inline int JS_SetPropertyRecursive(JSContext* ctx, JSValue obj, const char* name, JSValue value) {
    const char* dot = strchr(name, '.');
    if (dot) {
      std::string first_property_name(name, dot - name);
      JSValue first_property = JS_GetPropertyStr(ctx, obj, first_property_name.c_str());
      if (JS_IsException(first_property)) {
        JS_FreeValue(ctx, value);
        return -1;
      }
      int result = JS_SetPropertyRecursive(ctx, first_property, dot + 1, value);
      JS_FreeValue(ctx, first_property);
      return result;
    } else {
      return JS_SetPropertyStr(ctx, obj, name, value);
    }
  }
}

#endif
