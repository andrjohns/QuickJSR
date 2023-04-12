inlineCxxPlugin <- function(...) {
    plugin <-
        Rcpp::Rcpp.plugin.maker(
                  include.before = "#include <quickjs.h>",
                  libs           = ldflags(to_console = FALSE),
                  package        = "QuickJSR"
              )
    settings <- plugin()
    settings$env$PKG_CPPFLAGS <- cxxflags(to_console = FALSE)
    settings
}
