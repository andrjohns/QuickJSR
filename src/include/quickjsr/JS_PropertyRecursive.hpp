#ifndef QUICKJSR_JS_GET_PROPERTY_RECURSIVE_HPP
#define QUICKJSR_JS_GET_PROPERTY_RECURSIVE_HPP

#include <quickjs-libc.h>
#include <string>
#include <cstring>

namespace quickjsr {
  JSValue JS_GetPropertyRecursive(JSContext* ctx, JSValue obj, const char* name) {
    const char* dot = strchr(name, '.');
    if (dot) {
      // The name contains a ".", so we extract the first property and recurse on the rest of the name
      std::string first_property_name(name, dot - name);
      JSValue first_property = JS_GetPropertyStr(ctx, obj, first_property_name.c_str());
      JSValue result = JS_GetPropertyRecursive(ctx, first_property, dot + 1);
      JS_FreeValue(ctx, first_property);
      return result;
    } else {
      // The name does not contain a ".", so we get the property from the object
      return JS_GetPropertyStr(ctx, obj, name);
    }
  }
  
  int JS_SetPropertyRecursive(JSContext* ctx, JSValue obj, const char* name, JSValue value) {
    const char* dot = strchr(name, '.');
    if (dot) {
      // The name contains a ".", so we extract the first property and recurse on the rest of the name
      std::string first_property_name(name, dot - name);
      JSValue first_property = JS_GetPropertyStr(ctx, obj, first_property_name.c_str());
      int result = JS_SetPropertyRecursive(ctx, first_property, dot + 1, value);
      JS_FreeValue(ctx, first_property);
      return result;
    } else {
      // The name does not contain a ".", so we set the property on the object
      return JS_SetPropertyStr(ctx, obj, name, value);
    }
  }
}

#endif
