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
    qjs_call(ContextList$context, function_name, args_to_json(...))
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
