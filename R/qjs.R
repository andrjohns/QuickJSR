#' qjs_eval
#'
#' Evaluate a single Javascript expression.
#'
#' @param eval_string A single string of the expression to evaluate
#' @return The result of the provided expression, the return type is
#'          mapped from JS to R using `jsonlite::fromJSON()`
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
  parse_return(.Call(`qjs_eval_`, eval_string))
}

qjs_context <- function(stack_size) {
  .Call(`qjs_context_`, stack_size)
}

qjs_source <- function(ctx_ptr, code_string) {
  .Call(`qjs_source_`, ctx_ptr, code_string)
}

qjs_call <- function(ctx_ptr, function_name, args_json) {
  parse_return(.Call(`qjs_call_`, ctx_ptr, function_name, args_json))
}

qjs_validate <- function(ctx_ptr, function_name) {
  .Call(`qjs_source_`, ctx_ptr, function_name)
}
