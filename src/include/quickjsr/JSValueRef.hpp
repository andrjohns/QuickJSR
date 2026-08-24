#ifndef QUICKJSR_JSVALUEREF_HPP
#define QUICKJSR_JSVALUEREF_HPP

#include "quickjs.h"
#include <cpp11.hpp>

namespace quickjsr {
  struct JSValueRefData {
    JSContext* ctx;
    JSValue value;
    JSValue receiver;
  };

  inline JSValueRefData* get_value_ref(SEXP x) {
    if (TYPEOF(x) != EXTPTRSXP || !Rf_inherits(x, "JSValueRef")) {
      cpp11::stop("Expected a JSValueRef");
    }
    JSValueRefData* ref = static_cast<JSValueRefData*>(R_ExternalPtrAddr(x));
    if (!ref) {
      cpp11::stop("JSValueRef is no longer valid");
    }
    return ref;
  }
}

#endif
