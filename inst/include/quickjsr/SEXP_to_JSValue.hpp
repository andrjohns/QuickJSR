#ifndef QUICKJSR_SEXP_TO_JSVALUE_HPP
#define QUICKJSR_SEXP_TO_JSVALUE_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <iostream>

namespace quickjsr {
  // Forward declaration to allow for recursive calls
  JSValue SEXP_to_JSValue(JSContext* ctx, SEXP x, bool auto_unbox);

  JSValue SEXP_to_JSValue_elem(JSContext* ctx, SEXP x, int i, bool auto_unbox) {
    switch(TYPEOF(x)) {
      case REALSXP:
        return JS_NewFloat64(ctx, REAL(x)[i]);
      case INTSXP:
        return JS_NewInt32(ctx, INTEGER(x)[i]);
      case LGLSXP:
        return JS_NewBool(ctx, LOGICAL(x)[i]);
      case STRSXP:
        return JS_NewString(ctx, CHAR(STRING_ELT(x, i)));
      case VECSXP:
        return SEXP_to_JSValue(ctx, VECTOR_ELT(x, i), auto_unbox);
      default:
        return JS_UNDEFINED;
    }
  }

  JSValue SEXP_to_JSValue_array(JSContext* ctx, SEXP x, bool auto_unbox) {
    JSValue arr = JS_NewArray(ctx);
    for (int i = 0; i < Rf_length(x); i++) {
      JSValue val = SEXP_to_JSValue_elem(ctx, x, i, auto_unbox);
      JS_SetPropertyUint32(ctx, arr, i, val);
    }
    return arr;
  }

  JSValue SEXP_to_JSValue_object(JSContext* ctx, SEXP x, bool auto_unbox) {
    JSValue obj = JS_NewObject(ctx);
    for (int i = 0; i < Rf_length(x); i++) {
      SEXP name = STRING_ELT(Rf_getAttrib(x, R_NamesSymbol), i);
      JSValue val = SEXP_to_JSValue_elem(ctx, x, i, auto_unbox);
      JS_SetPropertyStr(ctx, obj, CHAR(name), val);
    }
    return obj;
  }

  JSValue SEXP_to_JSValue(JSContext* ctx, SEXP x, bool auto_unbox = false) {
    // Following jsonlite conventions:
    //   - R list with names is an object, otherwise an array
    if (Rf_isNewList(x)) {
      if (Rf_getAttrib(x, R_NamesSymbol) != R_NilValue) {
        return SEXP_to_JSValue_object(ctx, x, auto_unbox);
      } else {
        return SEXP_to_JSValue_array(ctx, x, auto_unbox);
      }
    }
    if (Rf_isArray(x)) {
      return SEXP_to_JSValue_array(ctx, x, auto_unbox);
    }
    if (Rf_length(x) == 1 && auto_unbox) {
      return SEXP_to_JSValue_elem(ctx, x, 0, true);
    } else {
      return SEXP_to_JSValue_array(ctx, x, true);
    }
  }

} // namespace quickjsr

#endif
