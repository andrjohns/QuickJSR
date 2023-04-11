inlineCxxPlugin <- function(...) {
    plugin <-
        Rcpp::Rcpp.plugin.maker(
                  include.before = "#include <quickjs.h>",
                  libs           = ldflags(as_character = TRUE),
                  package        = "QuickJSR"
              )
    settings <- plugin()
    settings$env$PKG_CPPFLAGS <- cxxflags(as_character = TRUE)
    settings
}
