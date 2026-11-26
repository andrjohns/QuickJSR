#' Persistent JavaScript value references
#'
#' @description
#' Retain values inside their JavaScript context so chained operations do not
#' convert intermediate values to R. References keep their context alive and can
#' only be passed back to that same context.
#'
#' @param ref A `JSValueRef` created by a `JSContext`.
#' @param name A property name or dotted property path.
#' @param ... Arguments passed to the referenced JavaScript function.
#' @return `js_ref_get()` and `js_ref_call()` return a `JSValueRef`.
#'   `js_ref_to_r()` materialises the referenced value as an R object.
#'
#' @examples
#' ctx <- JSContext$new()
#' ref <- ctx$eval_ref("({ value: 2, add(x) { return this.value + x; } })")
#' result <- js_ref_call(js_ref_get(ref, "add"), 3)
#' js_ref_to_r(result)
#'
#' @rdname JSValueRef
#' @aliases JSValueRef
#' @export
js_ref_get <- function(ref, name) {
  .Call(`qjs_value_ref_get_`, ref, name)
}

#' @rdname JSValueRef
#' @export
js_ref_call <- function(ref, ...) {
  .Call(`qjs_value_ref_call_`, ref, list(...))
}

#' @rdname JSValueRef
#' @export
js_ref_to_r <- function(ref) {
  .Call(`qjs_value_ref_to_r_`, ref)
}

#' Create an R ALTREP view over a JavaScript typed array
#'
#' @description
#' Creates a zero-copy R view over the storage of a retained JavaScript typed
#' array. `Uint8Array` and `Uint8ClampedArray` produce raw vectors,
#' `Int32Array` produces an integer vector, and `Float64Array` produces
#' a double vector.
#'
#' @details
#' The backing ArrayBuffer is made permanently immutable before its address is
#' exposed to R. This affects every JavaScript alias of that buffer. Indexed
#' JavaScript writes are ignored, while mutating typed-array methods, DataView
#' writes, resizing, and transfer operations throw an error.
#'
#' The ALTREP object retains the reference and its context while it reads the
#' JavaScript buffer. When R requests writable storage, the values are copied
#' into an ordinary R vector, the R modification affects only that copy, and the
#' JavaScript reference can be released.
#'
#' Shared, detached, unsupported, and out-of-bounds typed-array buffers are
#' rejected. An `Int32Array` value of `-2147483648` is R's
#' `NA_integer_` sentinel. Float64 NaN payloads are preserved, including an
#' R `NA_real_` payload originally transferred to JavaScript.
#'
#' ALTREP-aware R operations can read the buffer without copying. Other R code
#' may request writable or materialised storage and therefore trigger a copy.
#'
#' @param ref A `JSValueRef` containing a supported typed array.
#' @return An ALTREP raw, integer, or double vector backed by the JavaScript
#'   ArrayBuffer.
#'
#' @examples
#' ctx <- JSContext$new(profile = "bare")
#' ref <- ctx$eval_ref("new Float64Array([1, 2, 3, 4])")
#' values <- js_ref_to_altrep(ref)
#' sum(values)
#' values[1]
#'
#' @export
js_ref_to_altrep <- function(ref) {
  .Call(`qjs_value_ref_to_altrep_`, ref)
}

#' Compile and repeatedly execute JavaScript
#'
#' @description
#' Compiled scripts avoid parsing source on every execution. The bytecode remains
#' owned by its original context and can be executed repeatedly. The script keeps
#' that context alive.
#'
#' @details
#' Compilation does not execute the source. Compiled scripts cannot be passed as
#' ordinary JavaScript values. For parameterised work, evaluate a JavaScript
#' function with `ctx$eval_ref()` and call it repeatedly with
#' `js_ref_call()`.
#'
#' @param script A script created by `ctx$compile(code)`.
#' @return `js_script_run()` converts the result to R. `js_script_run_ref()`
#'   returns a `JSValueRef` owned by the script's context.
#'
#' @examples
#' ctx <- JSContext$new(profile = "bare")
#' ctx$eval("globalThis.value = 0")
#' script <- ctx$compile("value += 1; value")
#' js_script_run(script)
#' js_script_run(script)
#'
#' @rdname JSCompiledScript
#' @aliases JSCompiledScript
#' @export
js_script_run <- function(script) {
  .Call(`qjs_script_run_`, script)
}

#' @rdname JSCompiledScript
#' @export
js_script_run_ref <- function(script) {
  .Call(`qjs_script_run_ref_`, script)
}

#' @export
print.JSValueRef <- function(x, ...) {
  cat("<QuickJSR JavaScript reference>\n")
  invisible(x)
}

#' @export
print.JSCompiledScript <- function(x, ...) {
  cat("<QuickJSR compiled JavaScript script>\n")
  invisible(x)
}
