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

#' Get a variable from the current context
#'
#' @name JSContext-method-get
#' @aliases get
#'
#' @usage get(var_name)
#'
#' @description
#' Get the value of a variable from the current context
#'
#' @param var_name The name of the variable to retrieve
#' @return The value of the variable
#'
#' @examples
#' \dontrun{
#' ctx <- JSContext$new()
#' ctx$source(code = "var a = 1;")
#' ctx$get("a")
#' }
get <- NULL

#' Assign a value to a variable in the current context
#'
#' @name JSContext-method-assign
#' @aliases assign
#'
#' @usage assign(var_name, value)
#'
#' @description
#' Assign a value to a variable in the current context
#'
#' @param var_name The name of the variable to assign
#' @param value The value to assign to the variable
#' @return No return value, called for side effects
#'
#' @examples
#' \dontrun{
#' ctx <- JSContext$new()
#' ctx$assign("a", 1)
#' ctx$get("a")
#' }
assign <- NULL

#' Create persistent JavaScript references
#'
#' @name JSContext-reference-methods
#' @aliases JSContext-method-call_ref JSContext-method-get_ref JSContext-method-eval_ref call_ref get_ref eval_ref
#'
#' @usage
#' call_ref(function_name, ...)
#'
#' get_ref(var_name)
#'
#' eval_ref(code)
#'
#' @description
#' Evaluate, retrieve, or call JavaScript while retaining the result inside the
#' context instead of eagerly converting it to R.
#'
#' @param function_name The global function or dotted method path to call.
#' @param ... Arguments passed to the JavaScript function.
#' @param var_name The global value or dotted property path to retrieve.
#' @param code JavaScript source code to evaluate.
#' @return A `JSValueRef` owned by the context.
call_ref <- NULL
get_ref <- NULL
eval_ref <- NULL

#' Compile code in a persistent JavaScript context
#'
#' @name JSContext-method-compile
#' @aliases compile
#'
#' @usage compile(code)
#'
#' @description
#' Compiles source code in an existing `JSContext` without executing it. Use
#' `ctx$compile(code)` to invoke the method.
#'
#' @param code JavaScript source code to compile.
#' @return A `JSCompiledScript` owned by the context.
#'
#' @seealso [js_script_run()]
#'
#' @examples
#' ctx <- JSContext$new(profile = "bare")
#' script <- ctx$compile("1 + 2")
#' js_script_run(script)
compile <- NULL

#' Evaluate code in a persistent JavaScript context
#'
#' @name JSContext-method-eval
#' @aliases eval
#'
#' @usage eval(code)
#'
#' @description
#' Evaluates source code in an existing `JSContext`. Global state remains
#' available to later evaluations, calls, and compiled scripts. Use
#' `ctx$eval(code)` to invoke the method.
#'
#' @param code JavaScript source code to evaluate.
#' @return The evaluation result converted to R.
eval <- NULL

#' Create a JavaScript context
#'
#' @name JSContext-new
#' @aliases new
#'
#' @usage new(stack_size = NULL, profile = c("host", "standard", "bare"))
#'
#' @description
#' Creates a persistent QuickJS runtime and context.
#'
#' @details
#' Create contexts through `JSContext$new()`.
#'
#' The `host` profile preserves the complete API with `std`, `os`,
#' console helpers, and the global `R` bridge. The `standard` profile
#' installs `std`, `os`, and console helpers without the global
#' `R` bridge. The `bare` profile installs only the core JavaScript
#' runtime. R values can still be passed through context methods in every profile.
#'
#' @param stack_size Optional maximum JavaScript stack size in bytes.
#' @param profile Context facilities to install. See Details.
#' @return A persistent `JSContext`.
#'
#' @examples
#' ctx <- JSContext$new(profile = "bare")
#' ctx$source(code = "function add(a, b) { return a + b; }")
#' ctx$call("add", 2, 3)
new <- NULL

new_JSContext <- function(stack_size = NULL, profile = c("host", "standard", "bare")) {
  stack_size_int <- ifelse(is.null(stack_size), -1, stack_size)
  profile <- match.arg(profile)
  profile_id <- switch(profile, bare = 0L, standard = 1L, host = 2L)
  # A single pointer owning both the runtime and its context, so that
  # freeing always happens in the correct order (context before runtime)
  # regardless of R's garbage collection order.
  ContextList <- list(
    context = qjs_context(stack_size_int, profile_id),
    profile = profile
  )

  ContextList$validate <- function(code_string) {
    qjs_validate(ContextList$context, code_string)
  }

  ContextList$source <- function(file = NULL, code = NULL) {
    eval_success <- TRUE
    if (!is.null(file)) {
      if (!is.null(code)) {
        warning(
          "Both a filepath and code string cannot be provided,",
          " code will be ignored!",
          call. = FALSE
        )
      }
      eval_success <- qjs_source(
        ContextList$context,
        input = normalizePath(file),
        is_file = TRUE
      )
    } else if (!is.null(code)) {
      eval_success <- qjs_source(
        ContextList$context,
        input = code,
        is_file = FALSE
      )
    } else {
      stop("No JS code provided!", call. = FALSE)
    }

    if (!eval_success) {
      stop("Evaluating JS code failed, see message above!", call. = FALSE)
    }
    invisible(NULL)
  }
  ContextList$call <- function(function_name, ...) {
    qjs_call(ContextList$context, function_name, ...)
  }
  ContextList$call_ref <- function(function_name, ...) {
    qjs_call_ref(ContextList$context, function_name, ...)
  }
  ContextList$get <- function(var_name) {
    qjs_get(ContextList$context, var_name)
  }
  ContextList$get_ref <- function(var_name) {
    qjs_get_ref(ContextList$context, var_name)
  }
  ContextList$eval <- function(code) {
    qjs_context_eval(ContextList$context, code)
  }
  ContextList$eval_ref <- function(code) {
    qjs_eval_ref(ContextList$context, code)
  }
  ContextList$compile <- function(code) {
    qjs_compile(ContextList$context, code)
  }
  ContextList$assign <- function(var_name, value) {
    qjs_assign(ContextList$context, var_name, value)
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
