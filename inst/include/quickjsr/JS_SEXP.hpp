#ifndef QUICKJSR_JS_SEXP_HPP
#define QUICKJSR_JS_SEXP_HPP

#include <cpp11.hpp>
#include <quickjs-libc.h>
#include <quickjsr/JSValue_to_SEXP.hpp>

// Need to redefine the JS_CFUNC_DEF macro as it uses C features
// (designated initializers) which are not supported in C++ (until C++20)
#define JS_CFUNC_DEF_CPP(name, length, func1) { \
  name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0, \
  { { length, JS_CFUNC_generic, { func1 } } } \
  }

#ifndef countof
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif

namespace quickjsr {
  JSValue SEXP_to_JSValue(JSContext* ctx, SEXP x, const bool auto_unbox, const int64_t index);

  JSClassID js_sexp_class_id = 100;
  JSClassID js_renv_class_id = 101;
  JSClassDef js_sexp_class_def = {
    "SEXP",
    nullptr // finalized
  };


  static JSValue js_fun_static(JSContext* ctx, JSValueConst this_val, int argc,
                                JSValueConst* argv, int magic, JSValue* data) {
    JSValue data_val = data[0];
    SEXP x = reinterpret_cast<SEXP>(JS_GetOpaque(data_val, js_sexp_class_id));
    JS_FreeValue(ctx, data_val);
    if (argc == 0) {
      return SEXP_to_JSValue(ctx, cpp11::function(x)(), true, -1);
    }
    cpp11::writable::list args(argc);
    for (int i = 0; i < argc; i++) {
      args[i] = JSValue_to_SEXP(ctx, argv[i]);
    }
    cpp11::function do_call = cpp11::package("base")["do.call"];
    return SEXP_to_JSValue(ctx, do_call(x, args), true, -1);
  }

  static JSValue js_renv_get_property(JSContext *ctx, JSValueConst this_val, JSAtom atom, JSValueConst receiver) {
    const char *property_name = JS_AtomToCString(ctx, atom);
    JS_FreeCString(ctx, property_name);
    SEXP x = reinterpret_cast<SEXP>(JS_GetOpaque(this_val, js_renv_class_id));
    cpp11::environment env(x);
    SEXP prop_raw = env[property_name];
    PROTECT(prop_raw);
    SEXP fun = TYPEOF(prop_raw) == PROMSXP ? Rf_eval(prop_raw, env) : prop_raw;
    PROTECT(fun);
    JSValue result = SEXP_to_JSValue(ctx, fun, true, -1);
    UNPROTECT(2);
    return result;
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
    SEXP pkg_ns;
    if (strcmp(package_name, "base") == 0) {
      pkg_ns = R_BaseEnv;
    } else {
      SEXP pkg_name_sexp = Rf_mkString(package_name);
      pkg_ns = R_FindNamespace(pkg_name_sexp);
    }
    PROTECT(pkg_ns);
    JSValue result = SEXP_to_JSValue(ctx, pkg_ns, true, -1);
    UNPROTECT(1);
    return result;
  }

  static const JSCFunctionListEntry js_r_funcs[] = {
    JS_CFUNC_DEF_CPP("package", 1, js_r_package),
  };

  static JSValue create_r_object(JSContext *ctx) {
    JSValue r_obj = JS_NewObject(ctx);
    if (JS_IsException(r_obj)) {
      return r_obj;
    }
    JS_SetPropertyFunctionList(ctx, r_obj, js_r_funcs, countof(js_r_funcs));
    return r_obj;
  }
}

#endif
