js_typed_array <- function(x) {
  if (!typeof(x) %in% c("raw", "integer", "double")) {
    stop("x must be a raw, integer, or double vector", call. = FALSE)
  }
  if (typeof(x) == "integer" && anyNA(x)) {
    stop("integer typed arrays cannot contain NA", call. = FALSE)
  }
  structure(x, class = c("quickjs_typed_array", class(x)))
}

js_masked_typed_array <- function(x) {
  if (!typeof(x) %in% c("logical", "integer", "double")) {
    stop("x must be a logical, integer, or double vector", call. = FALSE)
  }
  if (inherits(x, c("factor", "Date", "POSIXct", "POSIXt"))) {
    stop("factors and dates are not supported", call. = FALSE)
  }
  structure(list(value = x), class = "quickjs_masked_typed_array")
}

js_columnar_data_frame <- function(x, typed = TRUE) {
  if (!is.data.frame(x)) {
    stop("x must be a data frame", call. = FALSE)
  }
  if (!is.logical(typed) || length(typed) != 1L || is.na(typed)) {
    stop("typed must be TRUE or FALSE", call. = FALSE)
  }
  attr(x, "quickjs_typed_columns") <- typed
  structure(x, class = c("quickjs_columnar_data_frame", class(x)))
}
