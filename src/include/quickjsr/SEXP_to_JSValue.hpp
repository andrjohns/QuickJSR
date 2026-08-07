#ifndef QUICKJSR_SEXP_TO_JSVALUE_HPP
#define QUICKJSR_SEXP_TO_JSVALUE_HPP

#include "quickjs.h"
#include <quickjsr/JSValue_to_SEXP.hpp>
#include <quickjsr/JS_SEXP.hpp>
#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <vector>
#include <ctime>
#include <cmath>

#if R_VERSION < R_Version(4, 5, 0)
# define R_ClosureFormals(x) FORMALS(x)
# define Rf_isDataFrame(x) Rf_isFrame(x)
#endif

namespace quickjsr {
  // Forward declaration to allow for recursive calls
  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr);
  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr, int64_t index, bool is_factor = false, bool is_date_class = false);

  // Format a POSIXct numeric value (seconds since epoch) as an ISO 8601 string
  inline std::string format_posixct_iso(double val) {
    if (std::isnan(val) || !std::isfinite(val)) return "null";
    time_t t = static_cast<time_t>(val);
    double frac = val - static_cast<double>(t);
    if (frac < 0) { frac += 1.0; t -= 1; }

    struct tm utc_tm;
#ifdef _WIN32
    gmtime_s(&utc_tm, &t);
#else
    gmtime_r(&t, &utc_tm);
#endif

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
                       utc_tm.tm_year + 1900, utc_tm.tm_mon + 1, utc_tm.tm_mday,
                       utc_tm.tm_hour, utc_tm.tm_min, utc_tm.tm_sec);
    int ms = static_cast<int>(frac * 1000 + 0.5);
    len += snprintf(buf + len, sizeof(buf) - len, ".%03d", ms);
    snprintf(buf + len, sizeof(buf) - len, "Z");
    return std::string(buf);
  }

  inline JSValue SEXP_to_JSValue_array(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr) {
    const int64_t n = Rf_xlength(x);
    bool is_factor = Rf_inherits(x, "factor");
    bool is_date_class = Rf_inherits(x, "POSIXct") || Rf_inherits(x, "POSIXt") || Rf_inherits(x, "Date");
    std::vector<JSValue> jsvals(n);
    for (int64_t i = 0; i < n; i++) {
      jsvals[i] = SEXP_to_JSValue(ctx, x, auto_unbox, auto_unbox_curr, i, is_factor, is_date_class);
    }
    return JS_NewArrayFrom(ctx, jsvals.size(), jsvals.data());
  }

  inline JSValue SEXP_to_JSValue_object(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr) {
    // Built via JS_NewObject()+JS_SetPropertyStr() rather than
    // JS_NewObjectFromStr(): the latter assumes the fresh object's shape is
    // uniquely owned (an assert to that effect is compiled out under
    // -DNDEBUG), but a freshly created empty object's shape is normally
    // shared (hash-consed) with every other empty object of the same
    // prototype, so mutating it in place corrupts all of them.
    const int64_t n = Rf_xlength(x);
    JSValue obj = JS_NewObject(ctx);
    if (n == 0) {
      return obj;
    }
    SEXP names = Rf_getAttrib(x, R_NamesSymbol);
    PROTECT(names);
    bool is_factor = Rf_inherits(x, "factor");
    bool is_date_class = Rf_inherits(x, "POSIXct") || Rf_inherits(x, "POSIXt") || Rf_inherits(x, "Date");
    for (int64_t i = 0; i < n; i++) {
      JSValue value = SEXP_to_JSValue(ctx, x, auto_unbox, auto_unbox_curr, i, is_factor, is_date_class);
      JS_SetPropertyStr(ctx, obj, Rf_translateCharUTF8(STRING_ELT(names, i)), value);
    }
    UNPROTECT(1);
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
    PROTECT(col_names);
    SEXP row_names = Rf_getAttrib(x, R_RowNamesSymbol);
    PROTECT(row_names);
    const int64_t ncol = Rf_xlength(x);

    const int64_t nrow = ncol > 0 ? Rf_xlength(VECTOR_ELT(x, 0)) : 0;
    std::vector<JSValue> rtn_vals(nrow);
    for (int64_t i = 0; i < nrow; i++) {
      JSValue row_obj = JS_NewObject(ctx);

      for (int64_t j = 0; j < ncol; j++) {
        SEXP col = VECTOR_ELT(x, j);
        JSValue col_val;
        if (Rf_isDataFrame(col)) {
          const int64_t nrow = Rf_xlength(col);
          SEXP df_names = Rf_getAttrib(col, R_NamesSymbol);
          PROTECT(df_names);
          JSValue dfcol_obj = JS_NewObject(ctx);
          bool is_factor = Rf_inherits(col, "factor");
          bool is_date_class = Rf_inherits(col, "POSIXct") || Rf_inherits(col, "POSIXt") || Rf_inherits(col, "Date");
          for (int64_t k = 0; k < nrow; k++) {
            JSValue dfcol_val = SEXP_to_JSValue(ctx, VECTOR_ELT(col, k), auto_unbox_inp, auto_unbox, i, is_factor, is_date_class);
            JS_SetPropertyStr(ctx, dfcol_obj, Rf_translateCharUTF8(STRING_ELT(df_names, k)), dfcol_val);
          }
          UNPROTECT(1);
          col_val = dfcol_obj;
        } else {
          bool is_factor = Rf_inherits(col, "factor");
          bool is_date_class = Rf_inherits(col, "POSIXct") || Rf_inherits(col, "POSIXt") || Rf_inherits(col, "Date");
          col_val = SEXP_to_JSValue(ctx, col, auto_unbox_inp, auto_unbox, i, is_factor, is_date_class);
        }
        JS_SetPropertyStr(ctx, row_obj, Rf_translateCharUTF8(STRING_ELT(col_names, j)), col_val);
      }

      // If row names are present and a character vector, add them to the object
      if (Rf_isString(row_names)) {
        JSValue row_name = JS_NewString(ctx, Rf_translateCharUTF8(STRING_ELT(row_names, i)));
        JS_SetPropertyStr(ctx, row_obj, "_row", row_name);
      }
      rtn_vals[i] = row_obj;
    }

    UNPROTECT(2);

    return JS_NewArrayFrom(ctx, rtn_vals.size(), rtn_vals.data());
  }

  static SEXP s_do_call = NULL;

  static JSValue js_fun_static(JSContext* ctx, JSValueConst this_val, int argc,
                                JSValueConst* argv, int magic, JSValue* data) {
    // data[0] is owned by the enclosing JSCFunctionDataRecord (freed once,
    // when the function object itself is finalized) and merely borrowed for
    // the duration of this call; it must not be freed here. Freeing it on
    // every call previously released the same reference repeatedly, leaving
    // a dangling opaque pointer after the second call.
    JSValue data_val = data[0];
    SEXP x = reinterpret_cast<SEXP>(JS_GetOpaque(data_val, js_sexp_class_id));
    if (argc == 0) {
      return SEXP_to_JSValue(ctx, cpp11::function(x)(), true, true);
    }
    if (!s_do_call) {
      s_do_call = Rf_findFun(Rf_install("do.call"), R_BaseEnv);
      R_PreserveObject(s_do_call);
    }
    cpp11::writable::list args(argc);
    for (int i = 0; i < argc; i++) {
      args[i] = JSValue_to_SEXP(ctx, argv[i]);
    }
    return SEXP_to_JSValue(ctx, cpp11::function(s_do_call)(x, args), true, true);
  }

  inline JSValue SEXP_to_JSValue_function(JSContext* ctx, const SEXP& x,
                                          bool auto_unbox_inp = false,
                                          bool auto_unbox = false) {
    // Nothing else protects `x` from R's GC once the .Call() that produced
    // it returns; keep it alive until js_sexp_finalizer() releases it.
    R_PreserveObject(x);
    JSValue obj = JS_NewObjectClass(ctx, js_sexp_class_id);
    JS_SetOpaque(obj, reinterpret_cast<void*>(x));
    // JS_NewCFunctionData() dups `obj` into its own storage, so the local
    // reference created above must still be freed here to avoid leaking it.
    JSValue fun = JS_NewCFunctionData(ctx, js_fun_static, Rf_xlength(R_ClosureFormals(x)),
                                       JS_CFUNC_generic, 1, &obj);
    JS_FreeValue(ctx, obj);
    return fun;
  }

  inline JSValue SEXP_to_JSValue_env(JSContext* ctx, const SEXP& x) {
    R_PreserveObject(x);
    JSValue obj = JS_NewObjectClass(ctx, js_renv_class_id);
    JS_SetOpaque(obj, reinterpret_cast<void*>(x));
    return obj;
  }


  inline JSValue SEXP_to_JSValue_matrix(JSContext* ctx, const SEXP& x, bool auto_unbox_inp = false, bool auto_unbox = false) {
    const int64_t nrow = Rf_nrows(x);
    const int64_t ncol = Rf_ncols(x);
    bool is_factor = Rf_inherits(x, "factor");
    bool is_date_class = Rf_inherits(x, "POSIXct") || Rf_inherits(x, "POSIXt") || Rf_inherits(x, "Date");
    std::vector<JSValue> row_vals(nrow);
    for (int64_t i = 0; i < nrow; i++) {
      std::vector<JSValue> values(ncol);
      for (int64_t j = 0; j < ncol; j++) {
        values[j] = SEXP_to_JSValue(ctx, x, auto_unbox_inp, auto_unbox, i + j * nrow, is_factor, is_date_class);
      }
      row_vals[i] = JS_NewArrayFrom(ctx, values.size(), values.data());
    }
    return JS_NewArrayFrom(ctx, row_vals.size(), row_vals.data());
  }

  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr, int64_t index, bool is_factor, bool is_date_class) {
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
        } else if (is_factor) {
          SEXP levels = Rf_getAttrib(x, R_LevelsSymbol);
          return JS_NewString(ctx, Rf_translateCharUTF8(STRING_ELT(levels, INTEGER_ELT(x, index) - 1)));
        } else {
          return JS_NewInt32(ctx, INTEGER_ELT(x, index));
        }
      }
      case REALSXP: {
        if (ISNA(REAL_ELT(x, index))) {
          return JS_NULL;
        } else if (is_date_class) {
          double val = REAL_ELT(x, index);
          if (Rf_inherits(x, "Date")) {
            val *= 86400.0;
          }
          std::string formatted = format_posixct_iso(val);
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
    bool is_factor = Rf_inherits(x, "factor");
    bool is_date_class = Rf_inherits(x, "POSIXct") || Rf_inherits(x, "POSIXt") || Rf_inherits(x, "Date");
    return SEXP_to_JSValue(ctx, x, auto_unbox_inp, auto_unbox_curr, 0, is_factor, is_date_class);
  }
} // namespace quickjsr

#endif
