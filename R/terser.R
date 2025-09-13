#' Minify JavaScript code using Terser
#'
#' This function minifies JavaScript code using the Terser library.
#'
#' @param code A character string containing the JavaScript code to be minified.
#' @param options A list of options to pass to Terser for minification.
#'   See the [Terser documentation](https://terser.org/docs/api-reference/#minify-options)
#'   for an up-to-date list of options and default values.
#' @return A list containing the minified code and any warnings or errors.
#' @examples
#' #' # Example usage
#' js_code <- "function add(a, b) { return a + b; }"
#' minified <- terser(js_code, list(sourceMap = TRUE))
#' minified$code
#' minified$map
#'
#' js_code <- "console.log(add(1 + 2, 3 + 4));"
#' options <- list(
#'   toplevel = TRUE,
#'   compress = list(
#'     passes = 2,
#'     global_defs = list(
#'       "@console.log" = "alert"
#'     )
#'   ),
#'   format = list(
#'     preamble = "/* minified */"
#'   )
#' )
#' terser(js_code, options)$code
#' @export
terser <- function(code, options = list()) {
  ctx_terser <- JSContext$new()
  ctx_terser$source(system.file("js", "terser.5.44.0.js", package = "QuickJSR"))
  ctx_terser$call("terser.minify_sync", code, options)
}
