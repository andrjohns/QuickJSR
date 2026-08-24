js_ref_get <- function(ref, name) {
  .Call(`qjs_value_ref_get_`, ref, name)
}

js_ref_call <- function(ref, ...) {
  .Call(`qjs_value_ref_call_`, ref, list(...))
}

js_ref_to_r <- function(ref) {
  .Call(`qjs_value_ref_to_r_`, ref)
}

print.JSValueRef <- function(x, ...) {
  cat("<QuickJSR JavaScript reference>\n")
  invisible(x)
}
