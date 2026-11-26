#' Transfer an R vector as a JavaScript typed array
#'
#' @description
#' Marks a vector for contiguous transfer to JavaScript. Raw vectors become
#' `Uint8Array`, integer vectors become `Int32Array`, and double vectors
#' become `Float64Array`. Integer vectors containing `NA` are rejected.
#' Double `NA` and `NaN` values both become JavaScript `NaN`.
#' On return, the JavaScript `Int32Array` value `-2147483648` becomes
#' R's `NA_integer_` sentinel.
#'
#' @param x A raw, integer, or double vector.
#' @return The input vector marked for typed-array conversion.
#'
#' @examples
#' ctx <- JSContext$new()
#' ctx$source(code = "function total(x) { return x.reduce((a, b) => a + b, 0); }")
#' ctx$call("total", js_typed_array(as.double(1:4)))
#'
#' @export
js_typed_array <- function(x) {
  if (!typeof(x) %in% c("raw", "integer", "double")) {
    stop("x must be a raw, integer, or double vector", call. = FALSE)
  }
  if (typeof(x) == "integer" && anyNA(x)) {
    stop("integer typed arrays cannot contain NA", call. = FALSE)
  }
  structure(x, class = c("quickjs_typed_array", class(x)))
}

#' Transfer missing typed values with a validity mask
#'
#' @description
#' Transfers an R vector as a branded JavaScript object containing contiguous
#' `values` and `validity` typed arrays. The validity byte is zero for
#' a missing value and one otherwise.
#'
#' @details
#' Logical values use a `Uint8Array`, integers use an `Int32Array`, and
#' doubles use a `Float64Array`. Every representation uses a
#' `Uint8Array` validity mask and exposes the original length as
#' `length`.
#'
#' R integer and logical `NA` values have validity zero and a placeholder
#' value of zero. Double `NA` is treated the same way, while ordinary
#' `NaN` remains a valid Float64 value. This preserves the R distinction
#' between `NA_real_` and `NaN`.
#'
#' JavaScript can modify the contents of both typed arrays. Returning the branded
#' object to R reconstructs the original R vector type using the current values
#' and validity bytes. A nonzero validity byte is treated as valid. Removing the
#' brand by constructing a plain object returns the two buffers separately under
#' the ordinary object conversion rules.
#'
#' The mask uses one byte per element. Use `js_typed_array()` when a mask is
#' not required. Factors and dates are rejected because their class semantics
#' cannot be represented by the numeric values and validity buffers alone.
#'
#' @param x A logical, integer, or double vector without factor or date classes.
#' @return A lightweight wrapper around the input vector.
#'
#' @examples
#' ctx <- JSContext$new(profile = "bare")
#' ctx$source(code = paste(
#'   "function fillMissing(x) {",
#'   "  x.values[1] = 20;",
#'   "  x.validity[1] = 1;",
#'   "  return x;",
#'   "}"
#' ))
#' values <- js_masked_typed_array(c(10L, NA_integer_, 30L))
#' ctx$call("fillMissing", values)
#'
#' @export
js_masked_typed_array <- function(x) {
  if (!typeof(x) %in% c("logical", "integer", "double")) {
    stop("x must be a logical, integer, or double vector", call. = FALSE)
  }
  if (inherits(x, c("factor", "Date", "POSIXct", "POSIXt"))) {
    stop("factors and dates are not supported", call. = FALSE)
  }
  structure(list(value = x), class = "quickjs_masked_typed_array")
}

#' Transfer a data frame as an object of columns
#'
#' @description
#' Transfers a data frame as one JavaScript object whose properties are columns.
#' This avoids creating one object per row. With `typed = TRUE`, raw,
#' integer, and double columns use contiguous typed arrays. Integer columns with
#' missing values, factors, and date-time columns use ordinary arrays. Double
#' `NA` and `NaN` values become JavaScript `NaN` in typed columns.
#' Explicit character row names are included as the `_row` column.
#'
#' @param x A data frame.
#' @param typed Use typed arrays for eligible raw, integer, and double columns.
#' @return The data frame marked for column-oriented conversion.
#'
#' @examples
#' ctx <- JSContext$new()
#' ctx$source(code = "function rows(x) { return x.value.length; }")
#' df <- data.frame(value = 1:4, label = letters[1:4])
#' ctx$call("rows", js_columnar_data_frame(df))
#'
#' @export
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
