#' ldflags
#'
#' Function for returning the flags needed for linking to the package
#'
#' @param to_console Whether the result should be returned as a string
#' @return Character string of linker flags, or print flags to console
#'         and invisibly return NULL (for use in package Makevars or similar)
#' @export
ldflags <- function(to_console = FALSE) {
  PKG_LIBS <- paste(
    "-L", shQuote(system.file("lib", Sys.getenv("R_ARCH"), package = "QuickJSR", mustWork = TRUE)),
    "-lquickjs"
  )
  if (isTRUE(to_console)) {
    cat(PKG_LIBS, " ")
    return(invisible(NULL))
  }
  PKG_LIBS
}

#' cxxflags
#'
#' Function for returning the C/C++ flags needed for compilation using the package's headers
#'
#' @param to_console Whether the result should be returned as a string
#' @return Character string of CXX flags, or print flags to console
#'         and invisibly return NULL (for use in package Makevars or similar)
#' @export
cxxflags <- function(to_console = FALSE) {
  CXXFLAGS <- paste(
    paste0("-I", shQuote(system.file("include", package = "QuickJSR", mustWork = TRUE))),
    "-D_GNU_SOURCE",
    "-DCONFIG_VERSION=\"2021-03-27\"",
    "-DSTRICT_R_HEADERS",
    "-DCONFIG_BIGNUM"
  )
  if (isTRUE(to_console)) {
    cat(CXXFLAGS, " ")
    return(invisible(NULL))
  }
  CXXFLAGS
}
