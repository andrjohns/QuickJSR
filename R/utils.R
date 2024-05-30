#' Get the version of the bundled QuickJS library
#'
#' @return Character string of the version of the bundled QuickJS library
#' @export
quickjs_version <- function() {
  version_file <- system.file("VERSION", package = "QuickJSR", mustWork = TRUE)
  readLines(version_file)
}

get_tz_offset_seconds <- function() {
  as.POSIXlt(Sys.time())$gmtoff
}
