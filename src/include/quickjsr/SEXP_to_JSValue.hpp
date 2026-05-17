#ifndef QUICKJSR_SEXP_TO_JSVALUE_HPP
#define QUICKJSR_SEXP_TO_JSVALUE_HPP

#include "quickjs.h"
#include <quickjsr/JSValue_to_SEXP.hpp>
#include <quickjsr/JS_SEXP.hpp>
#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <vector>

#if R_VERSION < R_Version(4, 5, 0)
# define R_ClosureFormals(x) FORMALS(x)
# define Rf_isDataFrame(x) Rf_isFrame(x)
#endif

namespace quickjsr {
  // Forward declaration to allow for recursive calls
  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr);
  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr, int64_t index);

  inline JSValue SEXP_to_JSValue_array(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr) {
    const int64_t n = Rf_xlength(x);
    std::vector<JSValue> jsvals(n);
    for (int64_t i = 0; i < n; i++) {
      jsvals[i] = SEXP_to_JSValue(ctx, x, auto_unbox, auto_unbox_curr, i);
    }
    return JS_NewArrayFrom(ctx, jsvals.size(), jsvals.data());
  }

  inline JSValue SEXP_to_JSValue_object(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr) {
    const int64_t n = Rf_xlength(x);
    SEXP names = Rf_getAttrib(x, R_NamesSymbol);
    std::vector<JSValue> values(n);
    std::vector<const char*> props(n);
    PROTECT(names);
    for (int64_t i = 0; i < n; i++) {
      values[i] = SEXP_to_JSValue(ctx, x, auto_unbox, auto_unbox_curr, i);
      props[i] = Rf_translateCharUTF8(STRING_ELT(names, i));
    }
    UNPROTECT(1);
    return JS_NewObjectFromStr(ctx, props.size(), props.data(), values.data());
  }

  inline JSValue SEXP_to_JSValue_list(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr) {
    // Following jsonlite conventions:
    //   - R list with names is an object, otherwise an array
    if (Rf_getAttrib(x, R_NamesSymbol) != R_NilValue) {
      return SEXP_to_JSValue_object(ctx, x, auto_unbox, auto_unbox_curr);
    } else {
      return SEXP_to_JSValue_array(ctx, x, auto_unbox, auto_unbox_curr);
    }
  }

  inline JSValue SEXP_to_JSValue_df(JSContext* ctx, const SEXP& x, bool auto_unbox_inp = false, bool auto_unbox = false) {
    SEXP col_names = Rf_getAttrib(x, R_NamesSymbol);
    PROTECT(col_names);
    SEXP row_names = Rf_getAttrib(x, R_RowNamesSymbol);
    PROTECT(row_names);
    const int64_t ncol = Rf_xlength(x);
    const int64_t obj_n = Rf_isString(row_names) ? ncol + 1 : ncol;

    const int64_t nrow = Rf_xlength(VECTOR_ELT(x, 0));
    std::vector<JSValue> rtn_vals(nrow);
    for (int64_t i = 0; i < nrow; i++) {
      std::vector<JSValue> row_vals(obj_n);
      std::vector<const char*> row_props(obj_n);

      for (int64_t j = 0; j < ncol; j++) {
        SEXP col = VECTOR_ELT(x, j);
        if (Rf_isDataFrame(col)) {
          const int64_t nrow = Rf_xlength(col);
          std::vector<JSValue> dfcol_vals(nrow);
          std::vector<const char*> dfcol_props(nrow);
          SEXP df_names = Rf_getAttrib(col, R_NamesSymbol);
          PROTECT(df_names);
          for (int64_t k = 0; k < nrow; k++) {
            dfcol_vals[k] = SEXP_to_JSValue(ctx, VECTOR_ELT(col, k), auto_unbox_inp, auto_unbox, i);
            dfcol_props[k] = Rf_translateCharUTF8(STRING_ELT(df_names, k));
          }
          UNPROTECT(1);
          row_vals[j] = JS_NewObjectFromStr(ctx, dfcol_props.size(), dfcol_props.data(), dfcol_vals.data());
        } else {
          row_vals[j] = SEXP_to_JSValue(ctx, col, auto_unbox_inp, auto_unbox, i);
        }
        row_props[j] = Rf_translateCharUTF8(STRING_ELT(col_names, j));
      }

      // If row names are present and a character vector, add them to the object
      if (Rf_isString(row_names)) {
        row_vals[ncol] = JS_NewString(ctx, Rf_translateCharUTF8(STRING_ELT(row_names, i)));
        row_props[ncol] = "_row";
      }
      rtn_vals[i] = JS_NewObjectFromStr(ctx, row_props.size(), row_props.data(), row_vals.data());
    }

    UNPROTECT(2);

    return JS_NewArrayFrom(ctx, rtn_vals.size(), rtn_vals.data());
  }

  static JSValue js_fun_static(JSContext* ctx, JSValueConst this_val, int argc,
                                JSValueConst* argv, int magic, JSValue* data) {
    JSValue data_val = data[0];
    SEXP x = reinterpret_cast<SEXP>(JS_GetOpaque(data_val, js_sexp_class_id));
    JS_FreeValue(ctx, data_val);
    if (argc == 0) {
      return SEXP_to_JSValue(ctx, cpp11::function(x)(), true, true);
    }
    cpp11::writable::list args(argc);
    for (int i = 0; i < argc; i++) {
      args[i] = JSValue_to_SEXP(ctx, argv[i]);
    }
    cpp11::function do_call = cpp11::package("base")["do.call"];
    return SEXP_to_JSValue(ctx, do_call(x, args), true, true);
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


  inline JSValue SEXP_to_JSValue_matrix(JSContext* ctx, const SEXP& x, bool auto_unbox_inp = false, bool auto_unbox = false) {
    const int64_t nrow = Rf_nrows(x);
    const int64_t ncol = Rf_ncols(x);
    std::vector<JSValue> row_vals(nrow);
    for (int64_t i = 0; i < nrow; i++) {
      std::vector<JSValue> values(ncol);
      for (int64_t j = 0; j < ncol; j++) {
        values[j] = SEXP_to_JSValue(ctx, x, auto_unbox_inp, auto_unbox, i + j * nrow);
      }
      row_vals[i] = JS_NewArrayFrom(ctx, values.size(), values.data());
    }
    return JS_NewArrayFrom(ctx, row_vals.size(), row_vals.data());
  }

  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr, int64_t index) {
    if (Rf_isDataFrame(x)) {
      return SEXP_to_JSValue_df(ctx, VECTOR_ELT(x, index), auto_unbox, auto_unbox_curr);
    }
    if (Rf_isNewList(x)) {
      return SEXP_to_JSValue(ctx, VECTOR_ELT(x, index), auto_unbox, auto_unbox_curr);
    }
    switch (TYPEOF(x)) {
      case NILSXP:
        return JS_NULL;
      case LGLSXP: {
        if (LOGICAL_ELT(x, index) == NA_LOGICAL) {
          return JS_NULL;
        }
        return JS_NewBool(ctx, LOGICAL_ELT(x, index));
      }
      case INTSXP: {
        if (INTEGER_ELT(x, index) == NA_INTEGER) {
          return JS_NULL;
        } else if (Rf_inherits(x, "factor")) {
          SEXP levels = Rf_getAttrib(x, R_LevelsSymbol);
          return JS_NewString(ctx, Rf_translateCharUTF8(STRING_ELT(levels, INTEGER_ELT(x, index) - 1)));
        } else {
          return JS_NewInt32(ctx, INTEGER_ELT(x, index));
        }
      }
      case REALSXP: {
        if (ISNA(REAL_ELT(x, index))) {
          return JS_NULL;
        } else if (Rf_inherits(x, "POSIXct") || Rf_inherits(x, "POSIXt") || Rf_inherits(x, "Date")) {
          cpp11::writable::doubles x_index(1);
          x_index[0] = REAL_ELT(x, index);
          // Match input classes
          x_index.attr("class") = Rf_getAttrib(x, R_ClassSymbol);
          cpp11::function format = cpp11::package("base")["format"];
          using cpp11::literals::operator""_nm;
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
          return JS_NewFloat64(ctx, REAL_ELT(x, index));
        }
      }
      case STRSXP: {
        if (STRING_ELT(x, index) == NA_STRING) {
          return JS_NULL;
        }
        return JS_NewString(ctx, Rf_translateCharUTF8(STRING_ELT(x, index)));
      }
      case VECSXP:
        return SEXP_to_JSValue(ctx, VECTOR_ELT(x, index), auto_unbox, auto_unbox_curr);
      case CLOSXP:
        return SEXP_to_JSValue_function(ctx, x, auto_unbox, auto_unbox_curr);
      case ENVSXP:
        return SEXP_to_JSValue_env(ctx, x);
      default:
        cpp11::stop("Conversions for type %s to JSValue are not yet implemented",
                    Rf_type2char(TYPEOF(x)));
    }
  }

  inline JSValue SEXP_to_JSValue_null(JSContext* ctx, bool auto_unbox) {
    if (auto_unbox) {
      return JS_NULL;
    } else {
      JSValue arr = JS_NewArray(ctx);
      JS_SetPropertyInt64(ctx, arr, 0, JS_NULL);
      return arr;
    }
  }

  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x,
                          bool auto_unbox_inp = false,
                          bool auto_unbox = false) {
    bool auto_unbox_curr = static_cast<bool>(Rf_inherits(x, "AsIs")) ? false : auto_unbox_inp;
    if (Rf_isNull(x)) {
      return SEXP_to_JSValue_null(ctx, auto_unbox_curr);
    }

    if (Rf_isDataFrame(x)) {
      return SEXP_to_JSValue_df(ctx, x, auto_unbox_inp, auto_unbox_curr);
    }
    if (Rf_isNewList(x)) {
      return SEXP_to_JSValue_list(ctx, x, auto_unbox_inp, auto_unbox_curr);
    }
    if (Rf_isMatrix(x)) {
      return SEXP_to_JSValue_matrix(ctx, x, auto_unbox_inp, auto_unbox_curr);
    }
    if (Rf_isVectorAtomic(x) || Rf_isArray(x)) {
      if (Rf_xlength(x) > 1 || !auto_unbox_curr || Rf_isArray(x)) {
        return SEXP_to_JSValue_array(ctx, x, auto_unbox_inp, auto_unbox_curr);
      }
    }
    return SEXP_to_JSValue(ctx, x, auto_unbox_inp, auto_unbox_curr, 0);
  }
} // namespace quickjsr

#endif
