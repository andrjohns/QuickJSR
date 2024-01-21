args_to_json <- function(...) {
  dots <- list(...)
  arg_names <- paste0("arg", seq_len(length(dots)))
  named_dots <- stats::setNames(dots, arg_names)
  jsonlite::toJSON(named_dots, auto_unbox = TRUE)
}

parse_return <- function(qjs_return) {
  if (qjs_return == "Error!") {
    stop("Error in JS runtime, see error message above for more information!",
          call. = FALSE)
  }
  jsonlite::fromJSON(qjs_return)
}

#' Get the version of the bundled QuickJS library
#'
#' @return Character string of the version of the bundled QuickJS library
#' @export
quickjs_version <- function() {
  version_file <- system.file("VERSION", package = "QuickJSR", mustWork = TRUE)
  readLines(version_file)
}
