#' ldflags
#'
#' Function for returning the flags needed for linking to the package
#'
#' @param to_console Whether the result should be returned as a string
#' @return Character string of linker flags, or print flags to console
#'         and invisibly return NULL (for use in package Makevars or similar)
#' @export
ldflags <- function(to_console = FALSE) {
  libdir <- system.file("lib", Sys.getenv("R_ARCH"), package = "QuickJSR",
                        mustWork = TRUE)
  pkglibs <- paste("-L", shQuote(libdir), "-lquickjs")
  if (isTRUE(to_console)) {
    cat(pkglibs, " ")
    return(invisible(NULL))
  }
  pkglibs
}

#' cxxflags
#'
#' Function for returning the C/C++ flags needed for compilation
#' using the package's headers
#'
#' @param to_console Whether the result should be returned as a string
#' @return Character string of CXX flags, or print flags to console
#'         and invisibly return NULL (for use in package Makevars or similar)
#' @export
cxxflags <- function(to_console = FALSE) {
  incdir <- system.file("include", package = "QuickJSR", mustWork = TRUE)
  pkg_cxxflags <- paste(
    paste0("-I", shQuote(incdir)),
    "-D_GNU_SOURCE",
    paste0("-DCONFIG_VERSION=\"", quickjs_version(), "\""),
    "-DSTRICT_R_HEADERS",
    "-DCONFIG_BIGNUM"
  )
  if (isTRUE(to_console)) {
    cat(pkg_cxxflags, " ")
    return(invisible(NULL))
  }
  pkg_cxxflags
}
