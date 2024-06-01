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

  static JSValue js_r_package(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc != 1) {
      return JS_ThrowTypeError(ctx, "R.package requires one argument");
    }

    const char *package_name = JS_ToCString(ctx, argv[0]);
    JS_FreeCString(ctx, package_name);
    if (!package_name) {
        return JS_EXCEPTION;
    }
    SEXP pkg = cpp11::package::get_namespace(package_name);
    return SEXP_to_JSValue(ctx, pkg, true, true);
  }

  static const JSCFunctionListEntry js_r_funcs[] = {
    JS_CFUNC_DEF("package", 1, js_r_package),
  };

  static JSValue create_r_object(JSContext *ctx) {
    JSValue r_obj = JS_NewObject
    (ctx);
    if (JS_IsException(r_obj)) {
      return r_obj;
    }
    JS_SetPropertyFunctionList(ctx, r_obj, js_r_funcs, countof(js_r_funcs));
    return r_obj;
  }
}

#endif
