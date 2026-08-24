#ifndef QUICKJSR_JS_GET_PROPERTY_RECURSIVE_HPP
#define QUICKJSR_JS_GET_PROPERTY_RECURSIVE_HPP

#include <quickjs-libc.h>
#include <vector>

namespace quickjsr {
  inline JSValue JS_GetPropertyRecursive(JSContext* ctx, JSValue obj,
                                         const std::vector<JSAtom>& path,
                                         JSValue* receiver = nullptr) {
    if (path.empty()) {
      return JS_EXCEPTION;
    }
    JSValue current = obj;
    bool owns_current = false;
    for (size_t i = 0; i < path.size(); i++) {
      if (i + 1 == path.size() && receiver) {
        *receiver = JS_DupValue(ctx, current);
      }
      JSValue next = JS_GetProperty(ctx, current, path[i]);
      if (owns_current) JS_FreeValue(ctx, current);
      if (JS_IsException(next)) return next;
      current = next;
      owns_current = true;
    }
    return current;
  }

  inline int JS_SetPropertyRecursive(JSContext* ctx, JSValue obj,
                                     const std::vector<JSAtom>& path,
                                     JSValue value) {
    if (path.empty()) {
      JS_FreeValue(ctx, value);
      return -1;
    }
    JSValue current = obj;
    bool owns_current = false;
    for (size_t i = 0; i + 1 < path.size(); i++) {
      JSValue next = JS_GetProperty(ctx, current, path[i]);
      if (owns_current) JS_FreeValue(ctx, current);
      if (JS_IsException(next)) {
        JS_FreeValue(ctx, value);
        return -1;
      }
      current = next;
      owns_current = true;
    }
    int result = JS_SetProperty(ctx, current, path.back(), value);
    if (owns_current) JS_FreeValue(ctx, current);
    return result;
  }
}

#endif
