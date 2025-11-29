#ifndef QUICKJSR_SEXP_TO_JSVALUE_HPP
#define QUICKJSR_SEXP_TO_JSVALUE_HPP

#include <quickjsr/JSValue_to_SEXP.hpp>
#include <quickjsr/JS_SEXP.hpp>
#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <type_traits>
#include <vector>

#if R_VERSION < R_Version(4, 5, 0)
# define R_ClosureFormals(x) FORMALS(x)
# define Rf_isDataFrame(x) Rf_isFrame(x)
#endif

namespace quickjsr {
  // Forward declaration to allow for recursive calls
  JSValue SEXP_to_JSValue(JSContext* ctx, SEXP x, const bool auto_unbox, const int64_t index = -1);

  template <typename Tval, typename Tmiss, typename std::enable_if<std::is_floating_point<Tval>::value>::type* = nullptr>
  bool is_NA(Tval&& value, Tmiss&& missing_value) {
    return R_IsNA(std::forward<Tval>(value)) || std::forward<Tval>(value) == std::forward<Tmiss>(missing_value);
  }

  template <typename Tval, typename Tmiss, typename std::enable_if<!std::is_floating_point<Tval>::value>::type* = nullptr>
  bool is_NA(Tval&& value, Tmiss&& missing_value) {
    return std::forward<Tval>(value) == std::forward<Tmiss>(missing_value);
  }

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
      auto&& val = index_func(std::forward<SexpT>(x), i);
      if (is_NA(std::forward<decltype(val)>(val), std::forward<MissingType>(missing_value))) {
        if (Rf_isNumeric(x) && !Rf_isLogical(x)) {
          values.push_back(JS_NewString(std::forward<CtxT>(ctx), "NA"));
        } else {
          values.push_back(JS_NULL);
        }
      } else {
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

  JSValue SEXP_to_JSValue_df(JSContext* ctx, SEXP x, const bool auto_unbox, int64_t index) {
    const int64_t rows = index == -1 ? Rf_xlength(VECTOR_ELT(x, 0)) : 1;
    SEXP col_names = Rf_getAttrib(x, R_NamesSymbol);
    PROTECT(col_names);
    SEXP row_names = Rf_getAttrib(x, R_RowNamesSymbol);
    PROTECT(row_names);
    const bool has_row_names = Rf_isString(row_names);
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
    const int64_t start = index == -1 ? 0 : index;
    const int64_t end = index == -1 ? rows : index + 1;
    for (int64_t i = start; i < end; i++) {
      std::vector<JSValue> row_values;
      row_values.reserve(cols);
      for (int64_t j = 0; j < cols; j++) {
        row_values.push_back(SEXP_to_JSValue(ctx, VECTOR_ELT(x, j), true, i));
      }
      if (has_row_names) {
        row_values.push_back(SEXP_to_JSValue(ctx, row_names, true, i));
      }
      values.push_back(JS_NewObjectFromStr(ctx, row_values.size(), props.data(), row_values.data()));
    }
    UNPROTECT(2);
    if (rows == 1 && auto_unbox) {
      return values[0];
    } else {
      return JS_NewArrayFrom(ctx, values.size(), values.data());
    }
  }

  JSValue SEXP_to_JSValue_vecsxp(JSContext* ctx, SEXP x, const bool auto_unbox, int64_t index) {
    if (Rf_isDataFrame(x)) {
      return SEXP_to_JSValue_df(ctx, x, auto_unbox, index);
    }
    const int64_t len = index == -1 ? Rf_xlength(x) : 1;
    SEXP names = Rf_getAttrib(x, R_NamesSymbol);
    PROTECT(names);
    // Ignore names if indexing a single element
    bool has_names = names != R_NilValue && index == -1;
    std::vector<JSValue> values;
    std::vector<const char*> props;
    values.reserve(len);
    props.reserve(len);
    const int64_t start = index == -1 ? 0 : index;
    const int64_t end = index == -1 ? len : index + 1;
    for (int64_t i = start; i < end; i++) {
      values.push_back(SEXP_to_JSValue(ctx, VECTOR_ELT(x, i), auto_unbox));
      if (has_names) {
        props.push_back(Rf_translateCharUTF8(STRING_ELT(names, i)));
      }
    }
    UNPROTECT(1);
    if (has_names) {
      return JS_NewObjectFromStr(ctx, values.size(), props.data(), values.data());
    } else {
      return JS_NewArrayFrom(ctx, values.size(), values.data());
    }
  }

  JSValue dbl_to_date(JSContext* ctx, double val, SEXP class_) {
    using cpp11::literals::operator""_nm;
    cpp11::writable::doubles x_index(1);
    x_index[0] = std::move(val);
    // Match input classes
    x_index.attr("class") = class_;
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
  }

  JSValue SEXP_to_JSValue(JSContext* ctx, SEXP x, const bool auto_unbox_inp, const int64_t index) {
    const int64_t len = Rf_xlength(x);
    if (Rf_isNull(x)) {
      return JS_NewObject(ctx);
    }
    if (len == 0) {
      return JS_NewArray(ctx);
    }
    if (Rf_isMatrix(x) && index == -1) {
      return SEXP_to_JSValue_matrix(ctx, x);
    }
    const bool auto_unbox = static_cast<bool>(Rf_inherits(x, "AsIs")) ? false : auto_unbox_inp;
    switch (TYPEOF(x)) {
      case NILSXP: {
        return JS_NewObject(ctx);
      }
      case LGLSXP: {
        const auto& index_func = [](const SEXP& x, const int64_t i) { return LOGICAL_ELT(x, i); };
        const auto& value_func = [](JSContext* ctx, const int val) { return JS_NewBool(ctx, val); };
        return SEXP_to_JSValue_prim(ctx, x, index_func, value_func, NA_LOGICAL, auto_unbox, index);
      }
      case INTSXP:  {
        const auto& index_func = [](const SEXP& x, const int64_t i) { return INTEGER_ELT(x, i); };
        const bool is_factor = Rf_inherits(x, "factor");
        SEXP levels = is_factor ? Rf_getAttrib(x, R_LevelsSymbol) : R_NilValue;
        const auto& value_func = [is_factor, levels](JSContext* ctx, int val) {
          return is_factor ? JS_NewString(ctx, Rf_translateCharUTF8(STRING_ELT(levels, val - 1))) : JS_NewInt32(ctx, val);
        };
        return SEXP_to_JSValue_prim(ctx, x, index_func, value_func, NA_INTEGER, auto_unbox, index);
      }
      case REALSXP: {
        const auto& index_func = [](const SEXP& x, const int64_t i) { return REAL_ELT(x, i); };
        const bool is_date = Rf_inherits(x, "Date") || Rf_inherits(x, "POSIXct") || Rf_inherits(x, "POSIXt");
        SEXP class_ = is_date ? Rf_getAttrib(x, R_ClassSymbol) : R_NilValue;
        const auto& value_func = [is_date, class_](JSContext* ctx, double val) {
          return is_date ? dbl_to_date(ctx, val, class_) : JS_NewFloat64(ctx, val);
        };
        return SEXP_to_JSValue_prim(ctx, x, index_func, value_func, NA_REAL, auto_unbox, index);
      }
      case STRSXP: {
        const auto& index_func = [](const SEXP& x, const int64_t i) { return STRING_ELT(x, i); };
        const auto& value_func = [](JSContext* ctx, const SEXP& val) { return JS_NewString(ctx, Rf_translateCharUTF8(val)); };
        return SEXP_to_JSValue_prim(ctx, x, index_func, value_func, NA_STRING, auto_unbox, index);
      }
      case VECSXP:
        return SEXP_to_JSValue_vecsxp(ctx, x, auto_unbox, index);
      case ENVSXP: {
        JSValue obj = JS_NewObjectClass(ctx, js_renv_class_id);
        JS_SetOpaque(obj, reinterpret_cast<void*>(x));
        return obj;
      }
      case CLOSXP: {
        JSValue obj = JS_NewObjectClass(ctx, js_sexp_class_id);
        JS_SetOpaque(obj, reinterpret_cast<void*>(x));
        return JS_NewCFunctionData(ctx, js_fun_static, Rf_xlength(R_ClosureFormals(x)),
                                    JS_CFUNC_generic, 1, &obj);
      }
      default:
        cpp11::stop("Conversions for type %s to JSValue are not yet implemented",
                    Rf_type2char(TYPEOF(x)));
    }
  }
} // namespace quickjsr

#endif
