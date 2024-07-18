#ifndef QUICKJSR_SEXP_TO_JSVALUE_HPP
#define QUICKJSR_SEXP_TO_JSVALUE_HPP

#include <quickjsr/utilities.hpp>
#include <quickjsr/JSValue_Date.hpp>
#include <quickjsr/JSValue_to_SEXP.hpp>
#include <quickjsr/JS_SEXP.hpp>
#include <cpp11.hpp>
#include <quickjs-libc.h>

#if R_VERSION < R_Version(4, 5, 0)
# define R_ClosureFormals(x) FORMALS(x)
# define Rf_isDataFrame(x) Rf_isFrame(x)
#endif

namespace quickjsr {
  // Forward declaration to allow for recursive calls
  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr);
  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr, int64_t index);

  inline JSValue SEXP_to_JSValue_array(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr) {
    JSValue arr = JS_NewArray(ctx);
    for (int64_t i = 0; i < Rf_xlength(x); i++) {
      JSValue val = SEXP_to_JSValue(ctx, x, auto_unbox, auto_unbox_curr, i);
      JS_SetPropertyInt64(ctx, arr, i, val);
    }
    return arr;
  }

  inline JSValue SEXP_to_JSValue_object(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr) {
    JSValue obj = JS_NewObject(ctx);
    SEXP names = Rf_getAttrib(x, R_NamesSymbol);
    for (int64_t i = 0; i < Rf_xlength(x); i++) {
      JSValue val = SEXP_to_JSValue(ctx, x, auto_unbox, auto_unbox_curr, i);
      JS_SetPropertyStr(ctx, obj, to_cstring(names, i), val);
    }
    return obj;
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
    SEXP row_names = Rf_getAttrib(x, R_RowNamesSymbol);
    JSValue arr = JS_NewArray(ctx);

    for (int64_t i = 0; i < Rf_xlength(VECTOR_ELT(x, 0)); i++) {
      JSValue obj = JS_NewObject(ctx);

      for (int64_t j = 0; j < Rf_xlength(x); j++) {
        SEXP col = VECTOR_ELT(x, j);
        if (Rf_isDataFrame(col)) {
          JSValue df_obj = JS_NewObject(ctx);
          SEXP df_names = Rf_getAttrib(col, R_NamesSymbol);
          for (int64_t k = 0; k < Rf_xlength(col); k++) {
            JSValue val = SEXP_to_JSValue(ctx, VECTOR_ELT(col, k), auto_unbox_inp, auto_unbox, i);
            JS_SetPropertyStr(ctx, df_obj, to_cstring(df_names, k), val);
          }
          JS_SetPropertyStr(ctx, obj, to_cstring(col_names, j), df_obj);
        } else {
          JSValue val = SEXP_to_JSValue(ctx, col, auto_unbox_inp, auto_unbox, i);
          JS_SetPropertyStr(ctx, obj, to_cstring(col_names, j), val);
        }
      }

      // If row names are present and a character vector, add them to the object
      if (Rf_isString(row_names)) {
        JSValue row_name = JS_NewString(ctx, to_cstring(row_names, i));
        JS_SetPropertyStr(ctx, obj, "_row", row_name);
      }

      JS_SetPropertyInt64(ctx, arr, i, obj);
    }

    return arr;
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
    int64_t nrow = Rf_nrows(x);
    int64_t ncol = Rf_ncols(x);
    JSValue arr = JS_NewArray(ctx);
    for (int64_t i = 0; i < nrow; i++) {
      JSValue row = JS_NewArray(ctx);
      for (int64_t j = 0; j < ncol; j++) {
        JSValue val = SEXP_to_JSValue(ctx, x, auto_unbox_inp, auto_unbox, i + j * nrow);
        JS_SetPropertyInt64(ctx, row, j, val);
      }
      JS_SetPropertyInt64(ctx, arr, i, row);
    }
    return arr;
  }

  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr, int64_t index) {
    if (Rf_isDataFrame(x)) {
      return SEXP_to_JSValue_df(ctx, VECTOR_ELT(x, index), auto_unbox, auto_unbox_curr);
    }
    if (Rf_isNewList(x)) {
      return SEXP_to_JSValue(ctx, VECTOR_ELT(x, index), auto_unbox, auto_unbox_curr);
    }
    switch (TYPEOF(x)) {
      case LGLSXP:
        return JS_NewBool(ctx, LOGICAL_ELT(x, index));
      case INTSXP: {
        if (Rf_inherits(x, "factor")) {
          SEXP levels = Rf_getAttrib(x, R_LevelsSymbol);
          return JS_NewString(ctx, to_cstring(levels, INTEGER_ELT(x, index) - 1));
        } else {
          return JS_NewInt32(ctx, INTEGER_ELT(x, index));
        }
      }
      case REALSXP: {
        if (Rf_inherits(x, "POSIXct")) {
          static constexpr double milliseconds_second = 1000;
          double tz_offset_seconds = get_tz_offset_seconds();
          return JS_NewDate(ctx, (REAL_ELT(x, index) + tz_offset_seconds) * milliseconds_second);
        } else if (Rf_inherits(x, "Date")) {
          static constexpr double milliseconds_day = 86400000;
          return JS_NewDate(ctx, REAL_ELT(x, index) * milliseconds_day);
        } else {
          return JS_NewFloat64(ctx, REAL_ELT(x, index));
        }
      }
      case STRSXP:
        return JS_NewString(ctx, to_cstring(x, index));
      case VECSXP:
        return SEXP_to_JSValue(ctx, VECTOR_ELT(x, index), auto_unbox, auto_unbox_curr);
      case CLOSXP:
        return SEXP_to_JSValue_function(ctx, x, auto_unbox, auto_unbox_curr);
      case ENVSXP:
        return SEXP_to_JSValue_env(ctx, x);
      case NILSXP:
        return JS_UNDEFINED;
      default:
        cpp11::stop("Conversions for type %s to JSValue are not yet implemented",
                    Rf_type2char(TYPEOF(x)));
    }
  }

  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x,
                          bool auto_unbox_inp = false,
                          bool auto_unbox = false) {
    bool auto_unbox_curr = static_cast<bool>(Rf_inherits(x, "AsIs")) ? false : auto_unbox_inp;
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
