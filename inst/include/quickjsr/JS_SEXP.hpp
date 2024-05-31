#ifndef QUICKJSR_JS_SEXP_HPP
#define QUICKJSR_JS_SEXP_HPP

#include <quickjs-libc.h>

// Define the JS class
JSClassID js_sexp_class_id;
JSClassDef js_sexp_class_def = {
    "SEXP",
    .finalizer = nullptr, // Add a finalizer if you need to clean up when the JSValue is garbage collected
};

#endif
