args_to_json <- function(...) {
  dots <- list(...)
  arg_names <- paste0("arg", seq_len(length(dots)))
  named_dots <- stats::setNames(dots, arg_names)
  jsonlite::toJSON(named_dots, auto_unbox = TRUE)
}

