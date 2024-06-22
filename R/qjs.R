#' qjs_eval
#'
#' Evaluate a single Javascript expression.
#'
#' @param eval_string A single string of the expression to evaluate
#' @return The result of the provided expression
#'
#' @examples
#' # Return the sum of two numbers:
#' qjs_eval("1 + 2")
#'
#' # Concatenate strings:
#' qjs_eval("'1' + '2'")
#'
#' # Create lists from objects:
#' qjs_eval("var t = {'a' : 1, 'b' : 2}; t")
#'
#' @export
qjs_eval <- function(eval_string) {
  .Call(`qjs_eval_`, eval_string)
}

qjs_context <- function(stack_size) {
  .Call(`qjs_context_`, stack_size)
}

qjs_source <- function(ctx_ptr, input, is_file) {
  .Call(`qjs_source_`, ctx_ptr, input, is_file)
}

qjs_call <- function(ctx_ptr, function_name, ...) {
  .Call(`qjs_call_`, ctx_ptr, function_name, list(...))
}

qjs_validate <- function(ctx_ptr, function_name) {
  .Call(`qjs_validate_`, ctx_ptr, function_name)
}

qjs_get <- function(ctx_ptr, var_name) {
  .Call(`qjs_get_`, ctx_ptr, var_name)
}

qjs_assign <- function(ctx_ptr, var_name, value) {
  res <- .Call(`qjs_assign_`, ctx_ptr, var_name, value)
  invisible(NULL)
}

#' to_json
#'
#' Use the QuickJS C API to convert an R object to a JSON string
#'
#' @param arg Argument to convert to JSON
#' @param auto_unbox Automatically unbox single element vectors
#' @return JSON string
#'
#' @export
to_json <- function(arg, auto_unbox = FALSE) {
  .Call(`to_json_`, arg, auto_unbox)
}

#' from_json
#'
#' Use the QuickJS C API to convert a JSON string to an R object
#'
#' @param json JSON string to convert to an R object
#' @return R object
#'
#' @export
from_json <- function(json) {
  .Call(`from_json_`, json)
}

#' Get the version of the bundled QuickJS library
#'
#' @return Character string of the version of the bundled QuickJS library
#' @export
quickjs_version <- function() {
  .Call(`qjs_version_`)
}
