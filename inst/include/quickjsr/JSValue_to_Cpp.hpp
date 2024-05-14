#ifndef QUICKJSR_JSVALUE_TO_CPP_HPP
#define QUICKJSR_JSVALUE_TO_CPP_HPP

#include <quickjsr/type_traits.hpp>
#include <type_traits>
#include <string>
#include <vector>
#include <quickjs-libc.h>

namespace quickjsr {
  template <typename T, std::enable_if_t<std::is_same<T, double>::value>* = nullptr>
  T JSValue_to_Cpp(JSContext* ctx, JSValue val) {
    double res;
    JS_ToFloat64(ctx, &res, val);
    return res;
  }

  template <typename T, std::enable_if_t<std::is_same<T, int32_t>::value>* = nullptr>
  T JSValue_to_Cpp(JSContext* ctx, JSValue val) {
    int32_t res;
    JS_ToInt32(ctx, &res, val);
    return res;
  }

  template <typename T, std::enable_if_t<std::is_same<T, bool>::value>* = nullptr>
  T JSValue_to_Cpp(JSContext* ctx, JSValue val) {
    return static_cast<bool>(JS_ToBool(ctx, val));
  }

  template <typename T, std::enable_if_t<std::is_same<T, std::string>::value>* = nullptr>
  T JSValue_to_Cpp(JSContext* ctx, JSValue val) {
    const char* res_str = JS_ToCString(ctx, val);
    std::string result = res_str;
    JS_FreeCString(ctx, res_str);
    return result;
  }

  template <typename T, std::enable_if_t<is_std_vector<T>::value>* = nullptr>
  T JSValue_to_Cpp(JSContext* ctx, JSValue val) {
    T res;
    uint32_t len;
    JSValue arr_len = JS_GetPropertyStr(ctx, val, "length");
    JS_ToUint32(ctx, &len, arr_len);
    JS_FreeValue(ctx, arr_len);
    for (uint32_t i = 0; i < len; i++) {
      JSValue elem = JS_GetPropertyUint32(ctx, val, i);
      res.push_back(JSValue_to_Cpp<value_type_t<T>>(ctx, elem));
      JS_FreeValue(ctx, elem);
    }
    return res;
  }

} // namespace quickjsr

#endif
