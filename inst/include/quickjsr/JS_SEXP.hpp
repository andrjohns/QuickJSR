#ifndef QUICKJSR_JS_SEXP_HPP
#define QUICKJSR_JS_SEXP_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <quickjsr/JSValue_to_SEXP.hpp>

namespace quickjsr {
  inline JSValue SEXP_to_JSValue(JSContext* ctx, const SEXP& x, bool auto_unbox, bool auto_unbox_curr);

  JSClassID js_sexp_class_id;
  JSClassID js_renv_class_id;
  JSClassDef js_sexp_class_def = {
    "SEXP",
    nullptr // finalized
  };

  static JSValue js_renv_get_property(JSContext *ctx, JSValueConst this_val, JSAtom atom, JSValueConst receiver) {
    const char *property_name = JS_AtomToCString(ctx, atom);
    JS_FreeCString(ctx, property_name);
    SEXP x = reinterpret_cast<SEXP>(JS_GetOpaque(this_val, js_renv_class_id));
    cpp11::environment env(x);
    return SEXP_to_JSValue(ctx, env[property_name], true, true);
  }

  static int js_renv_set_property(JSContext *ctx, JSValueConst this_val, JSAtom atom, JSValueConst value, JSValueConst receiver, int flags) {
    const char *property_name = JS_AtomToCString(ctx, atom);
    JS_FreeCString(ctx, property_name);
    SEXP x = reinterpret_cast<SEXP>(JS_GetOpaque(this_val, js_renv_class_id));
    cpp11::environment env(x);
    env[property_name] = JSValue_to_SEXP(ctx, value);
    return 0;
  }

  JSClassExoticMethods js_renv_exotic_methods = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    js_renv_get_property,
    js_renv_set_property
  };

  JSClassDef js_renv_class_def = {
    "REnv",
    nullptr,
    nullptr,
    nullptr,
    &js_renv_exotic_methods
  };
}

#endif
