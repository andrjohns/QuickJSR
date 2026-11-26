# QuickJSR 1.12.0
  * Update bundled QuickJS-NG engine to v0.18.0 (see https://github.com/quickjs-ng/quickjs/releases/tag/v0.18.0 for release notes)
    * `InternalError` and out-of-memory errors can no longer be caught by JS code
  * Add persistent JS references: `JSContext$get_ref()`, `$call_ref()`, `$eval_ref()`, with `js_ref_get()`, `js_ref_call()`, `js_ref_to_r()`
  * Add `JSContext$eval()` for evaluating code and returning the result
  * Add compiled scripts: `JSContext$compile()`, `js_script_run()`, `js_script_run_ref()`
  * Add `profile` argument to `JSContext$new()` (`"host"`, `"standard"`, `"bare"`)
  * Add `js_typed_array()` and `js_masked_typed_array()` for passing R vectors as JS typed arrays
  * Add `js_columnar_data_frame()` for passing data frames column-wise
  * Add `js_readonly_view()` and `js_mutable_view()` for exposing R vectors to JS without copying
  * Add `js_ref_to_altrep()` for ALTREP access to JS typed array buffers
  * Errors from active bindings in R environments accessed from JS now propagate as R errors
  * `JSContext$validate()` no longer evaluates the code being validated
  * `JSContext$call()` on an object method now binds `this` to the owning object
  * C++17 is now required
  * `tinytest (>= 1.4.1)` is now required for the test suite
  * Add pkgdown site at https://andrjohns.github.io/QuickJSR/

# QuickJSR 1.11.0
  * Update bundled QuickJS-NG engine to v0.16.2 (see https://github.com/quickjs-ng/quickjs/releases/tag/v0.16.2 for release notes)
  * Properly divert logging and IO through R interfaces
  * Resolve several bugs with R/JS object lifetime management

# QuickJSR 1.10.0
  * Update bundled QuickJS-NG engine to v0.14.0 (see https://github.com/quickjs-ng/quickjs/releases/tag/v0.14.0 for release notes)

# QuickJSR 1.9.0
  * Update bundled QuickJS-NG engine to v0.11.0 (see https://github.com/quickjs-ng/quickjs/releases for release notes)
  * Move build headers to reduce installed package size

# QuickJSR 1.8.1
  * Fix issue where `stack_size` option was ignored in `JSContext$new()`

# QuickJSR 1.8.0
  * Update bundled QuickJS-NG engine to v0.10.1 (see https://github.com/quickjs-ng/quickjs/releases for release notes)

# QuickJSR 1.7.0
  * Remove usage of disallowed entrypoints in compiled code

# QuickJSR 1.6.1
  * Update bundled QuickJS-NG engine to v0.9.0 (see https://github.com/quickjs-ng/quickjs/releases for release notes)

# QuickJSR 1.6.0
  * Sync bundled QuickJS-NG engine to 9d6e372 to address gcc-ubsan error
  * Patch bundled cpp11 headers for whitespace in literal operator declarations

# QuickJSR 1.5.2
  * Fix conversions of NULL/NA/undefined values between R and JS

# QuickJSR 1.5.1
  * Fix compilation under emscripten/WASM

# QuickJSR 1.5.0
  * Update bundled QuickJS-NG engine to v0.8.0 (see https://github.com/quickjs-ng/quickjs/releases for release notes)
  * Update vendored cpp11 headers to 0.5.1

# QuickJSR 1.4.0
  * Update bundled QuickJS-NG engine to v0.6.1 (see https://github.com/quickjs-ng/quickjs/releases for release notes)

# QuickJSR 1.3.1
  * Fix installation under R < 4.2

# QuickJSR 1.3.0
  * Bundled QuickJS engine updated to the QuickJS-NG fork, which is under more
    active development than the original QuickJS engine
  * Several Non-API R calls fixed
  * Unity/jumbo build implemented for QuickJS sources, allowing for faster
    compilation and improved compiler optimisations
  * Bugfixes for feature detection when system `CC` differs from `R CMD config CC`

# QuickJSR 1.2.2
  * Fix non-canonical CRAN URL in READMEE

# QuickJSR 1.2.1
  * Fix installation under C++11
  * Fix installation for FreeBSD
  * Fix detection of atomics support under Windows and ARM64
  * Fix module loading
  * Add `$get()` and `$assign()` methods to `JSContext`
  * Support passing R environments, getting and setting values
  * Add global R object with access to package environments

# QuickJSR 1.2.0
  * `Rcpp` dependency replaced with vendored `cpp11` headers
  * `R6` dependency removed
  * `R` and `JS` interoperability added, removing `jsonlite` dependency
  * Fixes for libatomic linking on 32-bit systems
  * Added `to_json` and `from_json` functions for testing `R`/`JS` interop

# QuickJSR 1.1.0
  * Fixed UBSAN error in `JS_Eval`
  * Fixed compilation errors with older GCC & Clang (`stdatomic.h not found`)

# QuickJSR 1.1.0
  * Bundled QuickJS engine updated to the 2024-01-13 release:
    - top-level-await support in modules
    - allow 'await' in the REPL
    - added Array.prototype.{with,toReversed,toSpliced,toSorted} and
    TypedArray.prototype.{with,toReversed,toSorted}
    - added String.prototype.isWellFormed and String.prototype.toWellFormed
    - added Object.groupBy and Map.groupBy
    - added Promise.withResolvers
    - class static block
    - 'in' operator support for private fields
    - optional chaining fixes
    - added RegExp 'd' flag
    - fixed RegExp zero length match logic
    - fixed RegExp case insensitive flag
    - added os.sleepAsync(), os.getpid() and os.now()
    - added cosmopolitan build
    - misc bug fixes

# QuickJSR 1.0.9
  * Bundled QuickJS engine updated to the 2023-12-09 release:
    - added Object.hasOwn, {String|Array|TypedArray}.prototype.at,
      {Array|TypedArray}.prototype.findLast{Index}
    - BigInt support is enabled even if CONFIG_BIGNUM disabled
    - updated to Unicode 15.0.0
    - misc bug fixes
