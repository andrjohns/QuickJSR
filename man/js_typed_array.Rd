\name{js_typed_array}
\alias{js_typed_array}
\title{Transfer an R vector as a JavaScript typed array}
\usage{
js_typed_array(x)
}
\arguments{
  \item{x}{A raw, integer, or double vector.}
}
\value{The input vector marked for typed-array conversion.}
\description{
Marks a vector for contiguous transfer to JavaScript. Raw vectors become
\code{Uint8Array}, integer vectors become \code{Int32Array}, and double vectors
become \code{Float64Array}. Integer vectors containing \code{NA} are rejected.
Double \code{NA} and \code{NaN} values both become JavaScript \code{NaN}.
On return, the JavaScript \code{Int32Array} value \code{-2147483648} becomes
R's \code{NA_integer_} sentinel.
}
\examples{
ctx <- JSContext$new()
ctx$source(code = "function total(x) { return x.reduce((a, b) => a + b, 0); }")
ctx$call("total", js_typed_array(as.double(1:4)))
}
