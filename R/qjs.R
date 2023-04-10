#' qjs_eval
#'
#' Evaluate a single Javascript expression.
#'
#' @param eval_string A single string of the expression to evaluate
#' @return The result of the evaluation. The return type is handled by
#'         the jsonlite::fromJSON function
#' @examples
#' # Return the sum of two numbers:
#' qjs_eval("1 + 2")
#'
#' # Concatenate strings:
#' qjs_eval("'1' + '2'")
#'
#' # Create lists from objects:
#' qjs_eval("{'a' : 1, 'b' : 2}")
#'
#' @export
qjs_eval <- function(eval_string) {
  jsonlite::fromJSON(.Call(`qjs_eval_`, eval_string))
}
