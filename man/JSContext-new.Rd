\name{JSContext-new}
\alias{JSContext-new}
\alias{new_JSContext}
\title{Create a JavaScript context}
\usage{
new_JSContext(
  stack_size = NULL,
  profile = c("host", "standard", "bare")
)
}
\arguments{
  \item{stack_size}{Optional maximum JavaScript stack size in bytes.}
  \item{profile}{Context facilities to install. See Details.}
}
\value{A persistent \code{JSContext}.}
\description{
Creates a persistent QuickJS runtime and context.
}
\details{
Create contexts through \code{JSContext$new()}.

The \code{host} profile preserves the complete API with \code{std}, \code{os},
console helpers, and the global \code{R} bridge. The \code{standard} profile
installs \code{std}, \code{os}, and console helpers without the global
\code{R} bridge. The \code{bare} profile installs only the core JavaScript
runtime. R values can still be passed through context methods in every profile.
}
\examples{
ctx <- JSContext$new(profile = "bare")
ctx$source(code = "function add(a, b) { return a + b; }")
ctx$call("add", 2, 3)
}
