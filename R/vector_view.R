js_readonly_view <- function(x) {
  if (!typeof(x) %in% c("raw", "logical", "integer", "double", "character")) {
    stop(
      "x must be a raw, logical, integer, double, or character vector",
      call. = FALSE
    )
  }
  structure(list(value = x), class = "quickjs_readonly_view")
}

js_mutable_view <- function(x) {
  if (!typeof(x) %in% c("raw", "logical", "integer", "double", "character") ||
      is.object(x)) {
    stop(
      "x must be an unclassed raw, logical, integer, double, or character vector",
      call. = FALSE
    )
  }
  structure(list(value = x[]), class = "quickjs_mutable_view")
}
