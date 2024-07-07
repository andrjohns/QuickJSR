#ifndef QUICKJSR_JSVALUE_TO_CPP_HPP
#define QUICKJSR_JSVALUE_TO_CPP_HPP

#include <quickjsr/type_traits.hpp>
#include <quickjsr/JSCommonType.hpp>
#include <type_traits>
#include <string>
#include <vector>
#include <quickjs-libc.h>

namespace quickjsr {
  template <typename T, enable_if_type_t<double, T>* = nullptr>
  T JSValue_to_Cpp(JSContext* ctx, JSValue val) {
    if (JS_IsDate(ctx, val)) {
      JSValue time = JS_GetTime(ctx, val);
      double res;
      JS_ToFloat64(ctx, &res, time);
      JS_FreeValue(ctx, time);
      return res / 1000.0; // Convert milliseconds to seconds for R POSIXct
    }

    double res;
    JS_ToFloat64(ctx, &res, val);
    return res;
  }

  template <typename T, enable_if_type_t<int32_t, T>* = nullptr>
  T JSValue_to_Cpp(JSContext* ctx, JSValue val) {
    int32_t res;
    JS_ToInt32(ctx, &res, val);
    return res;
  }

  template <typename T, enable_if_type_t<uint32_t, T>* = nullptr>
  T JSValue_to_Cpp(JSContext* ctx, JSValue val) {
    uint32_t res;
    JS_ToUint32(ctx, &res, val);
    return res;
  }

  template <typename T, enable_if_type_t<int64_t, T>* = nullptr>
  T JSValue_to_Cpp(JSContext* ctx, JSValue val) {
    int64_t res;
    JS_ToInt64(ctx, &res, val);
    return res;
  }

  template <typename T, enable_if_type_t<bool, T>* = nullptr>
  T JSValue_to_Cpp(JSContext* ctx, JSValue val) {
    return static_cast<bool>(JS_ToBool(ctx, val));
  }

  template <typename T, enable_if_type_t<std::string, T>* = nullptr>
  T JSValue_to_Cpp(JSContext* ctx, JSValue val) {
    const char* res_str = JS_ToCString(ctx, val);
    std::string result = res_str;
    JS_FreeCString(ctx, res_str);
    return result == "true" ? "TRUE" : result == "false" ? "FALSE" : result;
  }

  template <typename T, enable_if_type_t<const char*, T>* = nullptr>
  T JSValue_to_Cpp(JSContext* ctx, JSValue val) {
    return JSValue_to_Cpp<std::string>(ctx, val).c_str();
  }

  template <typename T, typename std::enable_if<is_std_vector<T>::value>::type* = nullptr>
  T JSValue_to_Cpp(JSContext* ctx, JSValue val) {
    T res;
    int64_t len;
    JS_GetLength(ctx, val, &len);
    res.reserve(len);
    for (int64_t i = 0; i < len; i++) {
      JSValue elem = JS_GetPropertyInt64(ctx, val, i);
      res.push_back(JSValue_to_Cpp<value_type_t<T>>(ctx, elem));
      JS_FreeValue(ctx, elem);
    }
    return res;
  }

} // namespace quickjsr

#endif
