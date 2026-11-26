#' Expose an R vector as a read-only JavaScript view
#'
#' @description
#' Creates an array-like JavaScript object that reads directly from retained R
#' vector storage. Creating the view does not copy or convert its elements.
#' Elements are converted only when JavaScript accesses them.
#'
#' @details
#' The view has `length`, indexed access, array iteration, and inherited
#' non-mutating `Array` methods. Use `view.toArray()` or
#' `Array.from(view)` when a mutable JavaScript array is required. Attempts
#' to write or delete indexed values throw an error.
#'
#' The wrapper does not copy the vector. The view retains the wrapper and vector
#' until JavaScript releases them. Normal R assignment follows R's copy-on-write
#' rules, so modifying an R binding can detach it from the storage retained by an
#' existing view. Returning the view to R recovers the wrapper without
#' materialising a JavaScript array.
#'
#' Missing values use the ordinary conversion rules: logical, integer, double,
#' and character `NA` become JavaScript `null`; `NaN` remains
#' `NaN`. Factors yield their labels, and dates yield JavaScript dates.
#'
#' @param x A raw, logical, integer, double, or character vector.
#' @return A lightweight wrapper around the input vector.
#'
#' @examples
#' ctx <- JSContext$new(profile = "bare")
#' ctx$source(code = paste(
#'   "function endpoints(x) { return [x[0], x[x.length - 1]]; }",
#'   "function total(x) { return x.reduce((a, b) => a + b, 0); }"
#' ))
#' values <- js_readonly_view(as.double(1:1000))
#' ctx$call("endpoints", values)
#' ctx$call("total", values)
#'
#' @export
js_readonly_view <- function(x) {
  if (!typeof(x) %in% c("raw", "logical", "integer", "double", "character")) {
    stop(
      "x must be a raw, logical, integer, double, or character vector",
      call. = FALSE
    )
  }
  structure(list(value = x), class = "quickjs_readonly_view")
}

#' Expose owned R vector storage as a mutable JavaScript view
#'
#' @description
#' Creates a fixed-length array-like JavaScript object whose indexed writes update
#' R-owned vector storage. The wrapper makes one copy when it is created; passing
#' it to JavaScript does not copy or convert the complete vector.
#'
#' @details
#' JavaScript reads convert elements on demand and writes update
#' `view$value`. The caller's original vector is never modified. Multiple
#' JavaScript views of the same wrapper share its owned storage.
#'
#' The view has `length`, indexed access, enumeration, iteration, inherited
#' `Array` methods, and `toArray()`. Its length cannot change, properties
#' cannot be added, and elements cannot be deleted. Methods such as `reverse`,
#' `sort`, `fill`, and `copyWithin` work because they only replace
#' existing elements.
#'
#' Assignments are type checked. JavaScript `null` and `undefined`
#' write R missing values except for raw vectors. JavaScript `NaN` remains an
#' ordinary R `NaN`. Factors, dates, and other classed vectors are rejected.
#'
#' Normal R copy-on-write rules still apply. Replacing or modifying
#' `view$value` in R after JavaScript retains the view can detach the R
#' binding from the retained storage.
#'
#' @param x An unclassed raw, logical, integer, double, or character vector.
#' @return A mutable-view wrapper containing an independent copy in `value`.
#'
#' @examples
#' ctx <- JSContext$new(profile = "bare")
#' ctx$source(code = "function increment(x) { x[0] += 1; return x; }")
#' values <- js_mutable_view(c(1, 2, 3))
#' ctx$call("increment", values)
#' values$value
#'
#' @export
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
