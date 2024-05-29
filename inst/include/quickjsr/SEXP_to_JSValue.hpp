#ifndef QUICKJSR_SEXP_TO_JSVALUE_HPP
#define QUICKJSR_SEXP_TO_JSVALUE_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <iostream>

namespace quickjsr {
  inline JSValue JS_NewDate(JSContext* ctx, double timestamp) {
    static constexpr double milliseconds_day = 86400000;
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue date_ctor = JS_GetPropertyStr(ctx, global_obj, "Date");
    JSValue timestamp_val = JS_NewFloat64(ctx, timestamp * milliseconds_day);
    JSValue date = JS_CallConstructor(ctx, date_ctor, 1, &timestamp_val);

    JS_FreeValue(ctx, global_obj);
    JS_FreeValue(ctx, date_ctor);
    JS_FreeValue(ctx, timestamp_val);
    return date;
  }
  // Forward declaration to allow for recursive calls
  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr);
  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr, int index);

  inline JSValue SEXP_to_JSValue_array(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr) {
    JSValue arr = JS_NewArray(ctx);
    for (int i = 0; i < Rf_length(x); i++) {
      JSValue val = SEXP_to_JSValue(ctx, x, auto_unbox, auto_unbox_curr, i);
      JS_SetPropertyUint32(ctx, arr, i, val);
    }
    return arr;
  }

  inline JSValue SEXP_to_JSValue_object(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr) {
    JSValue obj = JS_NewObject(ctx);
    for (int i = 0; i < Rf_length(x); i++) {
      JSValue val = SEXP_to_JSValue(ctx, x, auto_unbox, auto_unbox_curr, i);
      SEXP name = STRING_ELT(Rf_getAttrib(x, R_NamesSymbol), i);
      JS_SetPropertyStr(ctx, obj, Rf_translateCharUTF8(name), val);
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

  // For a dataframe with the first column of type list and the second column of type data.frame
  JSValue SEXP_to_JSValue_df(JSContext* ctx, const SEXP& x, bool auto_unbox_inp = false, bool auto_unbox = false) {
    SEXP col_names = Rf_getAttrib(x, R_NamesSymbol);
    SEXP row_names = Rf_getAttrib(x, R_RowNamesSymbol);
    JSValue arr = JS_NewArray(ctx);

    for (int i = 0; i < Rf_length(VECTOR_ELT(x, 0)); i++) {
      JSValue obj = JS_NewObject(ctx);

      for (int j = 0; j < Rf_length(x); j++) {
        SEXP col = VECTOR_ELT(x, j);
        if (Rf_isFrame(col)) {
          JSValue df_obj = JS_NewObject(ctx);
          SEXP df_names = Rf_getAttrib(col, R_NamesSymbol);
          for (int k = 0; k < Rf_length(col); k++) {
            JSValue val = SEXP_to_JSValue(ctx, VECTOR_ELT(col, k), auto_unbox_inp, auto_unbox, i);
            JS_SetPropertyStr(ctx, df_obj, Rf_translateCharUTF8(STRING_ELT(df_names, k)), val);
          }
          JS_SetPropertyStr(ctx, obj, Rf_translateCharUTF8(STRING_ELT(col_names, j)), df_obj);
        } else {
          JSValue val = SEXP_to_JSValue(ctx, col, auto_unbox_inp, auto_unbox, i);
          JS_SetPropertyStr(ctx, obj, Rf_translateCharUTF8(STRING_ELT(col_names, j)), val);
        }
      }

      // If row names are present and a character vector, add them to the object
      if (Rf_isString(row_names)) {
        JSValue row_name = JS_NewString(ctx, Rf_translateCharUTF8(STRING_ELT(row_names, i)));
        JS_SetPropertyStr(ctx, obj, "_row", row_name);
      }

      JS_SetPropertyUint32(ctx, arr, i, obj);
    }

    return arr;
  }

  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr, int index) {
    if (Rf_isFrame(x)) {
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
          return JS_NewString(ctx, Rf_translateCharUTF8(STRING_ELT(levels, INTEGER_ELT(x, index) - 1)));
        } else {
          return JS_NewInt32(ctx, INTEGER_ELT(x, index));
        }
      }
      case REALSXP: {
        if (Rf_inherits(x, "Date")) {
          return JS_NewDate(ctx, REAL_ELT(x, index));
        } else {
          return JS_NewFloat64(ctx, REAL_ELT(x, index));
        }
      }
      case STRSXP:
        return JS_NewString(ctx, Rf_translateCharUTF8(STRING_ELT(x, index)));
      case VECSXP:
        return SEXP_to_JSValue(ctx, VECTOR_ELT(x, index), auto_unbox, auto_unbox_curr);
      case NILSXP:
        return JS_UNDEFINED;
      default:
        cpp11::stop("Unsupported type for conversion to JSValue");
    }
  }

  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x,
                          bool auto_unbox_inp = false,
                          bool auto_unbox = false) {
    bool auto_unbox_curr = static_cast<bool>(Rf_inherits(x, "AsIs")) ? false : auto_unbox_inp;
    if (Rf_isFrame(x)) {
      return SEXP_to_JSValue_df(ctx, x, auto_unbox_inp, auto_unbox_curr);
    }
    if (Rf_isNewList(x)) {
      return SEXP_to_JSValue_list(ctx, x, auto_unbox_inp, auto_unbox_curr);
    }
    if (Rf_isVectorAtomic(x) || Rf_isArray(x)) {
      if (Rf_length(x) > 1 || !auto_unbox_curr || Rf_isArray(x)) {
        return SEXP_to_JSValue_array(ctx, x, auto_unbox_inp, auto_unbox_curr);
      }
    }
    return SEXP_to_JSValue(ctx, x, auto_unbox_inp, auto_unbox_curr, 0);

    cpp11::stop("Unsupported type for conversion to JSValue");
  }
} // namespace quickjsr

#endif
