#ifndef QUICKJSR_SEXP_TO_JSVALUE_HPP
#define QUICKJSR_SEXP_TO_JSVALUE_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>

namespace quickjsr {
  JSValue JS_NewDate(JSContext* ctx, double timestamp) {
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
  JSValue SEXP_to_JSValue(JSContext* ctx, SEXP x, bool auto_unbox, bool auto_unbox_curr);
  JSValue SEXP_to_JSValue(JSContext* ctx, SEXP x, bool auto_unbox, bool auto_unbox_curr, int index);

  JSValue SEXP_to_JSValue_array(JSContext* ctx, SEXP x, bool auto_unbox, bool auto_unbox_curr) {
    JSValue arr = JS_NewArray(ctx);
    for (int i = 0; i < Rf_length(x); i++) {
      JSValue val = SEXP_to_JSValue(ctx, x, auto_unbox, auto_unbox_curr, i);
      JS_SetPropertyUint32(ctx, arr, i, val);
    }
    return arr;
  }

  JSValue SEXP_to_JSValue_object(JSContext* ctx, SEXP x, bool auto_unbox, bool auto_unbox_curr) {
    JSValue obj = JS_NewObject(ctx);
    for (int i = 0; i < Rf_length(x); i++) {
      SEXP name = STRING_ELT(Rf_getAttrib(x, R_NamesSymbol), i);
      JSValue val = SEXP_to_JSValue(ctx, x, auto_unbox, auto_unbox_curr, i);
      JS_SetPropertyStr(ctx, obj, Rf_translateCharUTF8(name), val);
    }
    return obj;
  }

  JSValue SEXP_to_JSValue_df(JSContext* ctx, SEXP x, bool auto_unbox, bool auto_unbox_curr) {
    SEXP rownames = Rf_getAttrib(x, R_RowNamesSymbol);
    SEXP colnames = Rf_getAttrib(x, R_NamesSymbol);

    // Array of objects (one per row), if there are rownames
    // then they are placed at the end with name "_row"
    JSValue arr = JS_NewArray(ctx);
    for (int i = 0; i < Rf_length(rownames); i++) {
      JSValue obj = JS_NewObject(ctx);
      for (int j = 0; j < Rf_length(colnames); j++) {
        JSValue val = SEXP_to_JSValue(ctx, VECTOR_ELT(x, j), auto_unbox, auto_unbox_curr, i);
        JS_SetPropertyStr(ctx, obj, Rf_translateCharUTF8(STRING_ELT(colnames, j)), val);
      }
      // Only add rownames if they are character strings
      if (rownames != R_NilValue && Rf_isString(rownames)) {
        JSValue val = JS_NewString(ctx, Rf_translateCharUTF8(STRING_ELT(rownames, i)));
        JS_SetPropertyStr(ctx, obj, "_row", val);
      }
      JS_SetPropertyUint32(ctx, arr, i, obj);
    }
    return arr;
  }

  JSValue SEXP_to_JSValue_list(JSContext* ctx, SEXP x, bool auto_unbox, bool auto_unbox_curr) {
    // Following jsonlite conventions:
    //   - R list with names is an object, otherwise an array
    if (Rf_getAttrib(x, R_NamesSymbol) != R_NilValue) {
      return SEXP_to_JSValue_object(ctx, x, auto_unbox, auto_unbox_curr);
    } else {
      return SEXP_to_JSValue_array(ctx, x, auto_unbox, auto_unbox_curr);
    }
  }

  JSValue SEXP_to_JSValue(JSContext* ctx, SEXP x, bool auto_unbox, bool auto_unbox_curr, int index) {
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

  JSValue SEXP_to_JSValue(JSContext* ctx, SEXP x,
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
