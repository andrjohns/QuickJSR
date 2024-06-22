get_tz_offset_seconds <- function() {
  as.POSIXlt(Sys.time())$gmtoff
}
