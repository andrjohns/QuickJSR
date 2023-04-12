args_to_json <- function(...) {
  dots <- list(...)
  arg_names <- paste0("arg", seq_len(length(dots)))
  named_dots <- stats::setNames(dots, arg_names)
  jsonlite::toJSON(named_dots, auto_unbox = TRUE)
}

parse_return <- function(qjs_return) {
  error_strings = c(
    "Error in JSON.stringify()!",
    "Error initialising function!",
    "Error calling function!",
    "Error in evaluation!"
  )

  if (qjs_return %in% error_strings) {
    stop(qjs_return, call. = FALSE)
  }
  jsonlite::fromJSON(qjs_return)
}
