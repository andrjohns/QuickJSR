#' @title QuickJS-NG Compiler and Linker Flags
#' @name quickjs_flags
#' @description
#' Return the flags needed for compiling against the bundled
#' quickjs-ng headers and library
#'
#' @return A character string of flags
NULL

#' @rdname quickjs_flags
#' @export
cppflags <- function() {
  include_dir <- system.file("include", package = "QuickJSR", mustWork = TRUE)
  paste0("-I", shQuote(include_dir), " -D_GNU_SOURCE -funsigned-char")
}

#' @rdname quickjs_flags
#' @export
ldflags <- function() {
  lib_dir <- system.file("lib", Sys.getenv("R_ARCH"), package = "QuickJSR", mustWork = TRUE)
  paste0("-L", shQuote(lib_dir), " -lquickjs")
}
