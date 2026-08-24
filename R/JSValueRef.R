js_ref_get <- function(ref, name) {
  .Call(`qjs_value_ref_get_`, ref, name)
}

js_ref_call <- function(ref, ...) {
  .Call(`qjs_value_ref_call_`, ref, list(...))
}

js_ref_to_r <- function(ref) {
  .Call(`qjs_value_ref_to_r_`, ref)
}

js_ref_to_altrep <- function(ref) {
  .Call(`qjs_value_ref_to_altrep_`, ref)
}

js_script_run <- function(script) {
  .Call(`qjs_script_run_`, script)
}

js_script_run_ref <- function(script) {
  .Call(`qjs_script_run_ref_`, script)
}

print.JSValueRef <- function(x, ...) {
  cat("<QuickJSR JavaScript reference>\n")
  invisible(x)
}

print.JSCompiledScript <- function(x, ...) {
  cat("<QuickJSR compiled JavaScript script>\n")
  invisible(x)
}
