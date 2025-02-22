#ifndef QUICKJSR_JSVALUE_TO_SEXP_HPP
#define QUICKJSR_JSVALUE_TO_SEXP_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <quickjsr/JSValue_to_Cpp.hpp>
#include <quickjsr/JSCommonType.hpp>
#include <quickjsr/JSValue_Date.hpp>

namespace quickjsr {

// Forward declaration to allow for recursive calls
SEXP JSValue_to_SEXP(JSContext* ctx, const JSValue& val);

SEXP JSValue_to_SEXP_scalar(JSContext* ctx, const JSValue& val) {
  if (JS_IsNull(val)) {
    return R_NilValue;
  }
  if (JS_IsUndefined(val)) {
    return R_NilValue;
  }
  if (JS_IsBool(val)) {
    return cpp11::as_sexp(JSValue_to_Cpp<bool>(ctx, val));
  }
  if (JS_VALUE_GET_TAG(val) == JS_TAG_INT) {
    return cpp11::as_sexp(JSValue_to_Cpp<int32_t>(ctx, val));
  }
  if (JS_IsNumber(val)) {
    return cpp11::as_sexp(JSValue_to_Cpp<double>(ctx, val));
  }
  if (JS_IsString(val)) {
    return cpp11::as_sexp(JSValue_to_Cpp<std::string>(ctx, val));
  }
  if (JS_IsDate(ctx, val)) {
    cpp11::writable::doubles res = cpp11::as_sexp(JSValue_to_Cpp<double>(ctx, val));
    res.attr("class") = "POSIXct";
    return res;
  }
  return cpp11::as_sexp("Unsupported type");
}

SEXP JSValue_to_SEXP_vector(JSContext* ctx, const JSValue& val) {
  // Very inefficient workaround to identify null/undefined in array
  // so that they can be replaced with NA of the appropriate type in SEXP conversion
  std::vector<int64_t> null_idxs;
  int64_t len;
  JS_GetLength(ctx, val, &len);
  for (int64_t i = 0; i < len; i++) {
    JSValue elem = JS_GetPropertyInt64(ctx, val, i);
    if (JS_IsNull(elem) || JS_IsUndefined(elem)) {
      null_idxs.push_back(i);
    }
    JS_FreeValue(ctx, elem);
  }
  switch (JS_ArrayCommonType(ctx, val)) {
    case Integer: {
      SEXP int_sexp(cpp11::as_sexp(JSValue_to_Cpp<std::vector<int>>(ctx, val)));
      for (int64_t idx : null_idxs) {
        SET_INTEGER_ELT(int_sexp, idx, NA_INTEGER);
      }
      return int_sexp;
    }
    case Double: {
      SEXP dbl_sexp(cpp11::as_sexp(JSValue_to_Cpp<std::vector<double>>(ctx, val)));
      for (int64_t idx : null_idxs) {
        SET_REAL_ELT(dbl_sexp, idx, NA_REAL);
      }
      return dbl_sexp;
    }
    case Logical: {
      SEXP lgl_sexp(cpp11::as_sexp(JSValue_to_Cpp<std::vector<bool>>(ctx, val)));
      for (int64_t idx : null_idxs) {
        SET_LOGICAL_ELT(lgl_sexp, idx, NA_LOGICAL);
      }
      return lgl_sexp;
    }
    case Character: {
      SEXP chr_sexp(cpp11::as_sexp(JSValue_to_Cpp<std::vector<std::string>>(ctx, val)));
      for (int64_t idx : null_idxs) {
        SET_STRING_ELT(chr_sexp, idx, NA_STRING);
      }
      return chr_sexp;
    }
    case Undefined: {
      cpp11::writable::logicals res(len);
      for (int64_t i = 0; i < len; i++) {
        res[static_cast<R_xlen_t>(i)] = NA_LOGICAL;
      }
      return res;
    }
    case Date: {
      cpp11::writable::doubles res = cpp11::as_sexp(JSValue_to_Cpp<std::vector<double>>(ctx, val));
      for (int64_t idx : null_idxs) {
        SET_REAL_ELT(res, idx, NA_REAL);
      }
      res.attr("class") = "POSIXct";
      return res;
    }
    case NumberArray: {
      std::vector<std::vector<double>> res = JSValue_to_Cpp<std::vector<std::vector<double>>>(ctx, val);
      // Check that the inner vectors are all the same length
      size_t len = res[0].size();
      bool allSameSize = std::all_of(res.begin() + 1, res.end(),
                                    [len](const std::vector<double>& vec) { return vec.size() == len; });
      if (allSameSize) {
        cpp11::writable::doubles_matrix<cpp11::by_column> out(res.size(), len);
        for (size_t i = 0; i < res.size(); i++) {
          for (size_t j = 0; j < len; j++) {
            out(i, j) = res[i][j];
          }
        }
        return out;
      } else {
        cpp11::writable::list out(res.size());
        for (size_t i = 0; i < res.size(); i++) {
          out[static_cast<R_xlen_t>(i)] = cpp11::as_sexp(res[i]);
        }
        return out;
      }
    }
    case Object: {
      int64_t len;
      JS_GetLength(ctx, val, &len);

      cpp11::writable::list out(len);
      for (int64_t i = 0; i < len; i++) {
        JSValue elem = JS_GetPropertyInt64(ctx, val, i);
        out[static_cast<R_xlen_t>(i)] = JSValue_to_SEXP(ctx, elem);
        JS_FreeValue(ctx, elem);
      }
      return out;
    }
    default: {
      std::string type_str = "Unsupported type: ";
      // Get result of typeof
      JSValue typeof_val = JS_GetPropertyStr(ctx, val, "typeof");
      type_str += JSValue_to_Cpp<std::string>(ctx, typeof_val);
      JS_FreeValue(ctx, typeof_val);
      return cpp11::as_sexp(type_str.c_str());
    }
  }
}

SEXP JSValue_to_SEXP_list(JSContext* ctx, const JSValue& val) {
  // Get the keys of the object
  JSPropertyEnum* tab = NULL;
  uint32_t len = 0;
  JS_GetOwnPropertyNames(ctx, &tab, &len, val, JS_GPN_STRING_MASK);
  cpp11::writable::strings keys(len);
  cpp11::writable::list out(len);
  for (uint32_t i = 0; i < len; i++) {
    JSValue elem = JS_GetProperty(ctx, val, tab[i].atom);
    out[static_cast<R_xlen_t>(i)] = JSValue_to_SEXP(ctx, elem);

    const char* key = JS_AtomToCString(ctx, tab[i].atom);
    keys[static_cast<R_xlen_t>(i)] = key;

    JS_FreeValue(ctx, elem);
    JS_FreeCString(ctx, key);
  }
  JS_FreePropertyEnum(ctx, tab, len);
  out.attr("names") = keys;
  return out;
}

SEXP JSValue_to_SEXP(JSContext* ctx, const JSValue& val) {
  if (JS_IsException(val)) {
    js_std_dump_error(ctx);
    return cpp11::as_sexp("Error!");
  }
  if (JS_IsUndefined(val) || JS_IsNull(val)) {
    return R_NilValue;
  }
  if (JS_IsArray(ctx, val)) {
    return JSValue_to_SEXP_vector(ctx, val);
  }
  if (JS_IsObject(val) && !JS_IsDate(ctx, val)) {
    return JSValue_to_SEXP_list(ctx, val);
  }
  return JSValue_to_SEXP_scalar(ctx, val);
}

} // namespace quickjsr

#endif
