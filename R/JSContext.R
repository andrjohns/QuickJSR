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
JSContext <- R6::R6Class(
  classname = "JSContext",
  private = list(
    runtime_ = NULL,
    context_ = NULL
  ),
  public = list(
    #' @description
    #' Creates a new JSContext instance and initialises the QuickJS runtime and
    #' evaluation context
    #'
    #' @param stack_size An optional fixed value for the stack size (in bytes)
    #' @return No return value, used internally to initialise the JSContext object
    initialize = function(stack_size = NULL) {
      stack_size_int = ifelse(is.null(stack_size), -1, stack_size)
      rt_and_ctx = qjs_context(stack_size_int)
      private$runtime_ = rt_and_ctx$runtime_ptr
      private$context_ = rt_and_ctx$context_ptr
    },
    #' @description
    #' Checks whether a specified function or object is present in the initialised
    #' context.
    #'
    #' @param function_name The name of the function to check
    #' @return A boolean indicating whether the function is present
    validate = function(function_name) {
      qjs_validate(private$context_, function_name)
    },
    #' @description
    #' Evaluate a provided JavaScript file or string within the initialised context.
    #' Note that this method should only be used for initialising functions or values
    #' within the context, no values are returned from this function. See the `$call()`
    #' method for returning values.
    #'
    #' @param file A path to the JavaScript file to load
    #' @param code A single string of JavaScript to evaluate
    #' @return No return value, called for side effects
    source = function(file = NULL, code = NULL) {
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

      eval_success <- qjs_source(private$context_, code_string)
      if (!eval_success) {
        stop("Evaluating JS code failed!", call. = FALSE)
      }
      invisible(NULL)
    },
    #' @description
    #' Call a specified function in the JavaScript context with the
    #' provided arguments.
    #'
    #' @param function_name The function to be called
    #' @param ... The arguments to be passed to the function
    #' @return The result of calling the specified function,
    #'         the return type is mapped from JS to R using `jsonlite::fromJSON()`
    call = function(function_name, ...) {
      qjs_call(private$context_, function_name, args_to_json(...))
    })
)
