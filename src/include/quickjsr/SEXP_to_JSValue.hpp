#ifndef QUICKJSR_SEXP_TO_JSVALUE_HPP
#define QUICKJSR_SEXP_TO_JSVALUE_HPP

#include "quickjs.h"
#include <quickjsr/JSValue_to_SEXP.hpp>
#include <quickjsr/JS_SEXP.hpp>
#include <quickjsr/JSValueRef.hpp>
#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <algorithm>
#include <cmath>
#include <vector>

#if R_VERSION < R_Version(4, 5, 0)
# define R_ClosureFormals(x) FORMALS(x)
# define Rf_isDataFrame(x) Rf_isFrame(x)
#endif

namespace quickjsr {
  // Forward declaration to allow for recursive calls
  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr);
  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr, int64_t index, bool is_factor = false, bool is_date_class = false);

  inline JSValue SEXP_to_JSValue_typed_array(JSContext* ctx, const SEXP& x) {
    size_t size = static_cast<size_t>(Rf_xlength(x));
    if (TYPEOF(x) == RAWSXP) {
      return JS_NewUint8ArrayCopy(ctx, RAW(x), size);
    }

    JSTypedArrayEnum type;
    size_t bytes;
    if (TYPEOF(x) == INTSXP) {
      const int* values = INTEGER(x);
      if (size > 0 && std::find(values, values + size, NA_INTEGER) != values + size) {
        cpp11::stop("integer typed arrays cannot contain NA");
      }
      type = JS_TYPED_ARRAY_INT32;
      bytes = size * sizeof(int);
    } else if (TYPEOF(x) == REALSXP) {
      type = JS_TYPED_ARRAY_FLOAT64;
      bytes = size * sizeof(double);
    } else {
      cpp11::stop("typed arrays require raw, integer, or double vectors");
    }

    JSValue buffer = JS_NewArrayBufferCopy(
      ctx, static_cast<const uint8_t*>(DATAPTR_RO(x)), bytes
    );
    if (JS_IsException(buffer)) {
      return buffer;
    }
    JSValue args[3] = {buffer, JS_UNDEFINED, JS_UNDEFINED};
    JSValue result = JS_NewTypedArray(ctx, 3, args, type);
    JS_FreeValue(ctx, buffer);
    return result;
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
    const bool has_row_names = Rf_isString(row_names);
    const int64_t property_count = ncol + has_row_names;

    std::vector<SEXP> columns(ncol);
    std::vector<JSAtom> property_atoms(property_count);
    std::vector<uint8_t> factors(ncol);
    std::vector<uint8_t> dates(ncol);
    std::vector<uint8_t> data_frames(ncol);
    std::vector<std::vector<JSAtom>> nested_atoms(ncol);
    std::vector<std::vector<uint8_t>> nested_factors(ncol);
    std::vector<std::vector<uint8_t>> nested_dates(ncol);
    std::vector<std::vector<JSValue>> nested_values(ncol);

    for (int64_t j = 0; j < ncol; j++) {
      SEXP col = VECTOR_ELT(x, j);
      columns[j] = col;
      property_atoms[j] = JS_NewAtom(
        ctx, Rf_translateCharUTF8(STRING_ELT(col_names, j))
      );
      data_frames[j] = Rf_isDataFrame(col);
      factors[j] = Rf_inherits(col, "factor");
      dates[j] = Rf_inherits(col, "POSIXct") || Rf_inherits(col, "POSIXt") ||
        Rf_inherits(col, "Date");

      if (data_frames[j]) {
        const int64_t nested_count = Rf_xlength(col);
        SEXP names = Rf_getAttrib(col, R_NamesSymbol);
        nested_atoms[j].resize(nested_count);
        nested_factors[j].resize(nested_count);
        nested_dates[j].resize(nested_count);
        nested_values[j].resize(nested_count);
        for (int64_t k = 0; k < nested_count; k++) {
          SEXP nested = VECTOR_ELT(col, k);
          nested_atoms[j][k] = JS_NewAtom(
            ctx, Rf_translateCharUTF8(STRING_ELT(names, k))
          );
          nested_factors[j][k] = Rf_inherits(nested, "factor");
          nested_dates[j][k] = Rf_inherits(nested, "POSIXct") ||
            Rf_inherits(nested, "POSIXt") || Rf_inherits(nested, "Date");
        }
      }
    }
    if (has_row_names) {
      property_atoms[ncol] = JS_NewAtom(ctx, "_row");
    }

    std::vector<JSValue> rtn_vals(nrow);
    std::vector<JSValue> values(property_count);
    for (int64_t i = 0; i < nrow; i++) {
      for (int64_t j = 0; j < ncol; j++) {
        SEXP col = columns[j];
        if (data_frames[j]) {
          const int64_t nested_count = Rf_xlength(col);
          for (int64_t k = 0; k < nested_count; k++) {
            nested_values[j][k] = SEXP_to_JSValue(
              ctx, VECTOR_ELT(col, k), auto_unbox_inp, auto_unbox, i,
              nested_factors[j][k], nested_dates[j][k]
            );
          }
          values[j] = JS_NewObjectFrom(
            ctx, nested_count, nested_atoms[j].data(), nested_values[j].data()
          );
        } else {
          values[j] = SEXP_to_JSValue(
            ctx, col, auto_unbox_inp, auto_unbox, i, factors[j], dates[j]
          );
        }
      }
      if (has_row_names) {
        values[ncol] = JS_NewString(
          ctx, Rf_translateCharUTF8(STRING_ELT(row_names, i))
        );
      }
      rtn_vals[i] = JS_NewObjectFrom(
        ctx, property_count, property_atoms.data(), values.data()
      );
    }

    JSValue result = JS_NewArrayFrom(ctx, rtn_vals.size(), rtn_vals.data());
    for (JSAtom atom : property_atoms) JS_FreeAtom(ctx, atom);
    for (const auto& atoms : nested_atoms) {
      for (JSAtom atom : atoms) JS_FreeAtom(ctx, atom);
    }
    UNPROTECT(2);
    return result;
  }

  inline JSValue SEXP_to_JSValue_columnar_df(
    JSContext* ctx, const SEXP& x, bool auto_unbox_inp = false,
    bool auto_unbox = false
  ) {
    const int64_t ncol = Rf_xlength(x);
    SEXP names = Rf_getAttrib(x, R_NamesSymbol);
    SEXP row_names = Rf_getAttrib(x, R_RowNamesSymbol);
    const bool has_row_names = Rf_isString(row_names);
    const int64_t property_count = ncol + has_row_names;
    const bool typed = Rf_asLogical(Rf_getAttrib(
      x, Rf_install("quickjs_typed_columns")
    )) == TRUE;
    std::vector<JSAtom> atoms(property_count);
    std::vector<JSValue> values(property_count);

    for (int64_t j = 0; j < ncol; j++) {
      SEXP col = VECTOR_ELT(x, j);
      atoms[j] = JS_NewAtom(
        ctx, Rf_translateCharUTF8(STRING_ELT(names, j))
      );
      bool use_typed = typed && !Rf_inherits(col, "factor") &&
        !Rf_inherits(col, "POSIXct") && !Rf_inherits(col, "POSIXt") &&
        !Rf_inherits(col, "Date") &&
        (TYPEOF(col) == RAWSXP || TYPEOF(col) == INTSXP || TYPEOF(col) == REALSXP);
      if (use_typed && TYPEOF(col) == INTSXP) {
        const int* data = INTEGER(col);
        const R_xlen_t size = Rf_xlength(col);
        if (size > 0 && std::find(data, data + size, NA_INTEGER) != data + size) {
          use_typed = false;
        }
      }
      values[j] = use_typed
        ? SEXP_to_JSValue_typed_array(ctx, col)
        : SEXP_to_JSValue(ctx, col, auto_unbox_inp, auto_unbox);
    }
    if (has_row_names) {
      atoms[ncol] = JS_NewAtom(ctx, "_row");
      values[ncol] = SEXP_to_JSValue(
        ctx, row_names, auto_unbox_inp, auto_unbox
      );
    }

    JSValue result = JS_NewObjectFrom(
      ctx, property_count, atoms.data(), values.data()
    );
    for (JSAtom atom : atoms) JS_FreeAtom(ctx, atom);
    return result;
  }

  static JSValue js_fun_static(JSContext* ctx, JSValueConst this_val, int argc,
                                JSValueConst* argv, int magic, JSValue* data) {
    // data[0] is owned by the enclosing JSCFunctionDataRecord (freed once,
    // when the function object itself is finalized) and merely borrowed for
    // the duration of this call; it must not be freed here. Freeing it on
    // every call previously released the same reference repeatedly, leaving
    // a dangling opaque pointer after the second call.
    JSValue data_val = data[0];
    SEXP x = reinterpret_cast<SEXP>(JS_GetOpaque(data_val, js_sexp_class_id));

    SEXP call = PROTECT(Rf_allocLang(argc + 1));
    SETCAR(call, x);
    SEXP node = CDR(call);
    for (int i = 0; i < argc; i++) {
      SETCAR(node, JSValue_to_SEXP(ctx, argv[i]));
      node = CDR(node);
    }
    int error = 0;
    SEXP result = R_tryEvalSilent(call, R_GlobalEnv, &error);
    if (error) {
      std::string message = R_curErrorBuf();
      UNPROTECT(1);
      return JS_ThrowPlainError(ctx, "%s", message.c_str());
    }
    PROTECT(result);
    JSValue value = SEXP_to_JSValue(ctx, result, true, true);
    UNPROTECT(2);
    return value;
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
    std::vector<JSValue> values(ncol);
    for (int64_t i = 0; i < nrow; i++) {
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
          return JS_NewDate(ctx, val * 1000.0);
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
    if (Rf_inherits(x, "JSValueRef")) {
      JSValueRefData* ref = get_value_ref(x);
      if (ref->ctx != ctx) {
        cpp11::stop("JSValueRef belongs to a different context");
      }
      return JS_DupValue(ctx, ref->value);
    }
    if (Rf_inherits(x, "quickjs_typed_array")) {
      return SEXP_to_JSValue_typed_array(ctx, x);
    }
    if (Rf_inherits(x, "quickjs_columnar_data_frame")) {
      return SEXP_to_JSValue_columnar_df(ctx, x, auto_unbox_inp, auto_unbox);
    }
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
