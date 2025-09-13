.onLoad <- function(libname, pkgname) {
  # Initialise a new JS context for the terser library,
  # but do not load the code until the function is called.
  assign(".ctx_terser", JSContext$new(), envir = topenv())
  .ctx_terser$assign("terserLoaded", FALSE)
}

.onUnload <- function(libpath) {
  # unload the package library
  library.dynam.unload("QuickJSR", libpath)
}
