#' ldflags
#'
#' Function for returning the flags needed for linking to the package
#'
#' @param as_character Whether the result should be returned as a string
#' @export
ldflags <- function(as_character = FALSE) {
  PKG_LIBS <- paste(
    "-L", shQuote(system.file("lib", Sys.getenv("R_ARCH"), package = "QuickJSR", mustWork = TRUE)),
    "-lquickjs"
  )
  if (isTRUE(as_character)) return(PKG_LIBS)
  cat(PKG_LIBS, " ")
  return(invisible(NULL))
}

#' cxxflags
#'
#' Function for returning the C/C++ flags needed for compilation using the package's headers
#'
#' @param as_character Whether the result should be returned as a string
#' @export
cxxflags <- function(as_character = FALSE) {
  CXXFLAGS <- paste(
    paste0("-I", shQuote(system.file("include", package = "QuickJSR", mustWork = TRUE))),
    "-D_GNU_SOURCE",
    "-DCONFIG_VERSION=\"2021-03-27\"",
    "-DCONFIG_BIGNUM"
  )
  if (isTRUE(as_character)) return(CXXFLAGS)
  cat(CXXFLAGS, " ")
  return(invisible(NULL))
}
