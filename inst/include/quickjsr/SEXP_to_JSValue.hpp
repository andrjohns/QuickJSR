#ifndef QUICKJSR_SEXP_TO_JSVALUE_HPP
#define QUICKJSR_SEXP_TO_JSVALUE_HPP

#include <quickjsr/JSValue_to_SEXP.hpp>
#include <quickjsr/JS_SEXP.hpp>
#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <iostream>

#if R_VERSION < R_Version(4, 5, 0)
# define R_ClosureFormals(x) FORMALS(x)
# define Rf_isDataFrame(x) Rf_isFrame(x)
#endif

namespace quickjsr {
  // Forward declaration to allow for recursive calls
  JSValue SEXP_to_JSValue(JSContext* ctx, SEXP x, const bool auto_unbox, const int64_t index = -1);

  template <typename CtxT, typename SexpT, typename IndexFuncT, typename ValueFuncT, typename MissingType>
  JSValue SEXP_to_JSValue_prim(CtxT&& ctx, SexpT&& x,
                                         IndexFuncT&& index_func,
                                         ValueFuncT&& value_func,
                                         MissingType&& missing_value,
                                         const bool auto_unbox,
                                         const int64_t index) {
    const int64_t len = index == -1 ? Rf_xlength(x) : 1;
    std::vector<JSValue> values;
    values.reserve(len);
    const int64_t start = index == -1 ? 0 : index;
    const int64_t end = index == -1 ? len : index + 1;
    for (int64_t i = start; i < end; i++) {
      decltype(auto) val = index_func(std::forward<SexpT>(x), i);
      if (val != missing_value) {
        values.push_back(value_func(std::forward<CtxT>(ctx), std::forward<decltype(val)>(val)));
      }
    }
    if (values.size() == 0) {
      values.push_back(JS_NULL);
    }
    if (len == 1 && auto_unbox) {
      return values[0];
    } else {
      return JS_NewArrayFrom(ctx, values.size(), values.data());
    }
  }
  

  inline JSValue SEXP_to_JSValue_function(JSContext* ctx, const SEXP& x,
                                          bool auto_unbox_inp = false,
                                          bool auto_unbox = false) {
    JSValue obj = JS_NewObjectClass(ctx, js_sexp_class_id);
    JS_SetOpaque(obj, reinterpret_cast<void*>(x));
    return JS_NewCFunctionData(ctx, js_fun_static, Rf_xlength(R_ClosureFormals(x)),
                                JS_CFUNC_generic, 1, &obj);
  }

  inline JSValue SEXP_to_JSValue_env(JSContext* ctx, const SEXP& x) {
    JSValue obj = JS_NewObjectClass(ctx, js_renv_class_id);
    JS_SetOpaque(obj, reinterpret_cast<void*>(x));
    return obj;
  }

  JSValue SEXP_to_JSValue_vecsxp_df(JSContext* ctx, SEXP x, const bool auto_unbox) {
    const int64_t rows = Rf_xlength(VECTOR_ELT(x, 0));
    SEXP col_names = Rf_getAttrib(x, R_NamesSymbol);
    SEXP row_names = Rf_getAttrib(x, R_RowNamesSymbol);
    const bool has_row_names = row_names != R_NilValue;
    const int64_t cols = Rf_xlength(col_names);
    std::vector<JSValue> values;
    std::vector<const char*> props;
    values.reserve(rows);
    props.reserve(cols);
    for (int64_t i = 0; i < cols; i++) {
      props.push_back(Rf_translateCharUTF8(STRING_ELT(col_names, i)));
    }
    if (has_row_names) {
      props.push_back("_row");
    }
    for (int64_t i = 0; i < rows; i++) {
      std::vector<JSValue> row_values;
      row_values.reserve(cols);
      for (int64_t j = 0; j < cols; j++) {
        row_values.push_back(SEXP_to_JSValue(ctx, VECTOR_ELT(x, j), auto_unbox, i));
      }
      if (has_row_names) {
        row_values.push_back(SEXP_to_JSValue(ctx, row_names, auto_unbox, i));
      }
      values.push_back(JS_NewObjectFromStr(ctx, row_values.size(), props.data(), row_values.data()));
    }
    return JS_NewArrayFrom(ctx, values.size(), values.data());
  }

  JSValue SEXP_to_JSValue_matrix(JSContext* ctx, SEXP x) {
    const int64_t nrow = Rf_nrows(x);
    const int64_t ncol = Rf_ncols(x);
    std::vector<JSValue> values;
    values.reserve(nrow);
    for (int64_t i = 0; i < nrow; i++) {
      std::vector<JSValue> row_values;
      row_values.reserve(ncol);
      for (int64_t j = 0; j < ncol; j++) {
        JSValue val = SEXP_to_JSValue(ctx, x, true, i + j * nrow);
        if (JS_IsNull(val)) {
          val = JS_NewString(ctx, "NA");
        }
        row_values.push_back(val);
      }
      values.push_back(JS_NewArrayFrom(ctx, row_values.size(), row_values.data()));
    }
    return JS_NewArrayFrom(ctx, values.size(), values.data());
  }

  JSValue SEXP_to_JSValue_vecsxp(JSContext* ctx, SEXP x, const bool auto_unbox) {
    if (Rf_isDataFrame(x)) {
      return SEXP_to_JSValue_vecsxp_df(ctx, x, true);
    }
    const int64_t len = Rf_xlength(x);
    SEXP names = Rf_getAttrib(x, R_NamesSymbol);
    bool has_names = names != R_NilValue;
    std::vector<JSValue> values;
    std::vector<const char*> props;
    values.reserve(len);
    props.reserve(len);
    for (int64_t i = 0; i < len; i++) {
      values.push_back(SEXP_to_JSValue(ctx, VECTOR_ELT(x, i), auto_unbox));
      if (has_names) {
        props.push_back(Rf_translateCharUTF8(STRING_ELT(names, i)));
      }
    }
    if (len == 1 && auto_unbox && !has_names) {
      return values[0];
    } else {
      if (has_names) {
        return JS_NewObjectFromStr(ctx, values.size(), props.data(), values.data());
      } else {
        return JS_NewArrayFrom(ctx, values.size(), values.data());
      }
    }
  }

  JSValue SEXP_to_JSValue_nilsxp(JSContext* ctx, const bool auto_unbox) {
    if (auto_unbox) {
      return JS_NULL;
    } else {
      JSValue arr = JS_NewArray(ctx);
      JS_SetPropertyInt64(ctx, arr, 0, JS_NULL);
      return arr;
    }
  }

  JSValue SEXP_to_JSValue_lglsxp(JSContext* ctx, SEXP x, const bool auto_unbox, const int64_t index) {
    const auto& index_func = [](const SEXP& x, const int64_t i) { return LOGICAL_ELT(x, i); };
    const auto& value_func = [](JSContext* ctx, const int val) { return JS_NewBool(ctx, val); };
    return SEXP_to_JSValue_prim(ctx, x, index_func, value_func, NA_LOGICAL, auto_unbox, index);
  }

  JSValue SEXP_to_JSValue_intsxp(JSContext* ctx, SEXP x, const bool auto_unbox, const int64_t index) {
    const auto& index_func = [](const SEXP& x, const int64_t i) { return INTEGER_ELT(x, i); };
    const auto& value_func = [&x](JSContext* ctx, int val) {
      if (Rf_inherits(x, "factor")) {
        SEXP levels = Rf_getAttrib(x, R_LevelsSymbol);
        return JS_NewString(ctx, Rf_translateCharUTF8(STRING_ELT(levels, val - 1)));
      } else {
        return JS_NewInt32(ctx, val);
      }
    };
    return SEXP_to_JSValue_prim(ctx, x, index_func, value_func, NA_INTEGER, auto_unbox, index);
  }

  JSValue SEXP_to_JSValue_realsxp(JSContext* ctx, SEXP x, const bool auto_unbox, const int64_t index) {
    const auto& index_func = [](const SEXP& x, const int64_t i) { return REAL_ELT(x, i); };
    const auto& value_func = [&x](JSContext* ctx, double val) {
      if (Rf_inherits(x, "POSIXct") || Rf_inherits(x, "POSIXt") || Rf_inherits(x, "Date")) {
        cpp11::writable::doubles x_index(1);
        x_index[0] = std::move(val);
        // Match input classes
        x_index.attr("class") = Rf_getAttrib(x, R_ClassSymbol);
        cpp11::function format = cpp11::package("base")["format"];
        std::string formatted = cpp11::as_cpp<std::string>(format(x_index, "format"_nm = "%Y-%m-%dT%H:%M:%OSZ", "tz"_nm = "UTC"));
        // Create new Date from ISO string using JS_CallConstructor
        JSValue global = JS_GetGlobalObject(ctx);
        JSValue date_ctor = JS_GetPropertyStr(ctx, global, "Date");
        JSValue iso_str = JS_NewString(ctx, formatted.c_str());
        JSValue date_obj = JS_CallConstructor(ctx, date_ctor, 1, &iso_str);
        JS_FreeValue(ctx, iso_str);
        JS_FreeValue(ctx, date_ctor);
        JS_FreeValue(ctx, global);
        return date_obj;
      } else {
        return JS_NewFloat64(ctx, val);
      }
    };
    return SEXP_to_JSValue_prim(ctx, x, index_func, value_func, NA_REAL, auto_unbox, index);
  }

  JSValue SEXP_to_JSValue_strsxp(JSContext* ctx, SEXP x, const bool auto_unbox, const int64_t index) {
    const auto& index_func = [](const SEXP& x, const int64_t i) { return STRING_ELT(x, i); };
    const auto& value_func = [](JSContext* ctx, const SEXP& val) { return JS_NewString(ctx, Rf_translateCharUTF8(val)); };
    return SEXP_to_JSValue_prim(ctx, x, index_func, value_func, NA_STRING, auto_unbox, index);
  }

  JSValue SEXP_to_JSValue(JSContext* ctx, SEXP x, const bool auto_unbox, const int64_t index) {
    const int64_t len = Rf_xlength(x);
    if (len == 0) {
      return JS_NewArray(ctx);
    }
    if (Rf_isMatrix(x) && index == -1) {
      return SEXP_to_JSValue_matrix(ctx, x);
    }
    switch (TYPEOF(x)) {
      case NILSXP: 
        return SEXP_to_JSValue_nilsxp(ctx, auto_unbox);
      case LGLSXP: 
        return SEXP_to_JSValue_lglsxp(ctx, x, auto_unbox, index);
      case INTSXP: 
        return SEXP_to_JSValue_intsxp(ctx, x, auto_unbox, index);
      case REALSXP: 
        return SEXP_to_JSValue_realsxp(ctx, x, auto_unbox, index);
      case STRSXP: 
        return SEXP_to_JSValue_strsxp(ctx, x, auto_unbox, index);
      case VECSXP:
        return SEXP_to_JSValue_vecsxp(ctx, x, auto_unbox);
      case ENVSXP:
        return SEXP_to_JSValue_env(ctx, x);
      case CLOSXP:
        return SEXP_to_JSValue_function(ctx, x);
      default:
        cpp11::stop("Conversions for type %s to JSValue are not yet implemented",
                    Rf_type2char(TYPEOF(x)));
    }
  }
} // namespace quickjsr

#endif
