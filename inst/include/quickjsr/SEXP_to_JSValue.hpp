#ifndef QUICKJSR_SEXP_TO_JSVALUE_HPP
#define QUICKJSR_SEXP_TO_JSVALUE_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <iostream>

namespace quickjsr {

  JSValue SEXP_to_JSValue_scalar(JSContext* ctx, SEXP x, int i = 0) {
    switch(TYPEOF(x)) {
      case REALSXP:
        return JS_NewFloat64(ctx, REAL(x)[i]);
      case INTSXP:
        return JS_NewInt32(ctx, INTEGER(x)[i]);
      case LGLSXP:
        return JS_NewBool(ctx, LOGICAL(x)[i]);
      case STRSXP:
        return JS_NewString(ctx, CHAR(STRING_ELT(x, i)));
      default:
        return JS_UNDEFINED;
    }
  }

  JSValue SEXP_to_JSValue_vector(JSContext* ctx, SEXP x) {
    JSValue arr = JS_NewArray(ctx);
    for (int i = 0; i < Rf_length(x); i++) {
      JSValue val = SEXP_to_JSValue_scalar(ctx, x, i);
      JS_SetPropertyUint32(ctx, arr, i, val);
    }
    return arr;
  }

  JSValue SEXP_to_JSValue(JSContext* ctx, SEXP x, bool auto_unbox = false) {
    if (TYPEOF(x) == VECSXP) {
      cpp11::list x_list(x);
      if (x_list.named()) {
        JSValue obj = JS_NewObject(ctx);
        for (int i = 0; i < Rf_length(x); i++) {
          SEXP name = STRING_ELT(Rf_getAttrib(x, R_NamesSymbol), i);
          JSValue val = SEXP_to_JSValue(ctx, VECTOR_ELT(x, i));
          JS_SetPropertyStr(ctx, obj, CHAR(name), val);
        }
        return obj;
      } else {
        JSValue arr = JS_NewArray(ctx);
        for (int i = 0; i < Rf_length(x); i++) {
          JSValue val = SEXP_to_JSValue(ctx, VECTOR_ELT(x, i));
          JS_SetPropertyUint32(ctx, arr, i, val);
        }
        return arr;
      }
    }
    if (Rf_length(x) == 1 && auto_unbox) {
      return SEXP_to_JSValue_scalar(ctx, x);
    } else {
      return SEXP_to_JSValue_vector(ctx, x);
    }
  }

} // namespace quickjsr

#endif
