#' Assess validity of JS code without evaluating
#'
#' @name JSContext-method-validate
#' @aliases validate
#'
#' @usage validate(code_string)
#'
#' @description
#' Checks whether JS code string is valid code in the current context
#'
#' @param code_string The JS code to check
#' @return A boolean indicating whether code is valid
#'
#' @examples
#' \dontrun{
#' ctx <- JSContext$new()
#' ctx$validate("1 + 2")
#' }
validate <- NULL

#' Evaluate JS string or file in the current context
#'
#' @name JSContext-method-source
#' @aliases source
#'
#' @usage source(file = NULL, code = NULL)
#'
#' @description
#' Evaluate a provided JavaScript file or string within the initialised context.
#' Note that this method should only be used for initialising functions or values
#' within the context, no values are returned from this function. See the `$call()`
#' method for returning values.
#'
#' @param file A path to the JavaScript file to load
#' @param code A single string of JavaScript to evaluate
#' @return No return value, called for side effects
#'
#' @examples
#' \dontrun{
#' ctx <- JSContext$new()
#' ctx$source(file = "path/to/file.js")
#' ctx$source(code = "1 + 2")
#' }
source <- NULL

#' Call a JS function in the current context
#'
#' @name JSContext-method-call
#' @aliases call
#'
#' @usage call(function_name, ...)
#'
#' @description Call a specified function in the JavaScript context with the
#' provided arguments.
#'
#' @param function_name The function to be called
#' @param ... The arguments to be passed to the function
#' @return The result of calling the specified function
#'
#' @examples
#' \dontrun{
#' ctx <- JSContext$new()
#' ctx$source(code = "function add(a, b) { return a + b; }")
#' ctx$call("add", 1, 2)
#' }
call <- NULL

new_JSContext <- function(stack_size = NULL) {
  stack_size_int = ifelse(is.null(stack_size), -1, stack_size)
  rt_and_ctx = qjs_context(stack_size_int)
  ContextList = list(
    runtime = rt_and_ctx$runtime_ptr,
    context = rt_and_ctx$context_ptr
  )

  ContextList$validate <- function(code_string) {
    qjs_validate(ContextList$context, code_string)
  }

  ContextList$source <- function(file = NULL, code = NULL) {
    eval_success <- TRUE
    if (!is.null(file)) {
      if (!is.null(code)) {
        warning("Both a filepath and code string cannot be provided,",
                " code will be ignored!", call. = FALSE)
      }
      code_string <- paste0(readLines(con = file), collapse = "\n")
    } else if (!is.null(code)) {
      code_string <- code
    } else {
      stop("No JS code provided!", call. = FALSE)
    }
    eval_success <- qjs_source(ContextList$context, code_string)
    if (!eval_success) {
      stop("Evaluating JS code failed, see message above!", call. = FALSE)
    }
    invisible(NULL)
  }
  ContextList$call <- function(function_name, ...) {
    qjs_call(ContextList$context, function_name, ...)
  }
  structure(
    class = "JSContext",
    ContextList
  )
}

#' @title JSContext object
#'
#' @description
#'   An initialised context within which to evaluate Javascript
#'   scripts or commands.
#'
#' @return A JSContext object containing an initialised JavaScript
#'           context for evaluating scripts/commands
#'
#' @export
JSContext <- list(
  new = new_JSContext
)
