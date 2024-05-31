#ifndef QUICKJSR_JS_SEXP_HPP
#define QUICKJSR_JS_SEXP_HPP

#include <quickjs-libc.h>

namespace quickjsr {
  JSClassID js_sexp_class_id;
  JSClassDef js_sexp_class_def = {
      "SEXP",
      .finalizer = nullptr
  };
}

#endif
