\name{js_masked_typed_array}
\alias{js_masked_typed_array}
\title{Transfer missing typed values with a validity mask}
\usage{
js_masked_typed_array(x)
}
\arguments{
  \item{x}{A logical, integer, or double vector without factor or date classes.}
}
\value{A lightweight wrapper around the input vector.}
\description{
Transfers an R vector as a branded JavaScript object containing contiguous
\code{values} and \code{validity} typed arrays. The validity byte is zero for
a missing value and one otherwise.
}
\details{
Logical values use a \code{Uint8Array}, integers use an \code{Int32Array}, and
doubles use a \code{Float64Array}. Every representation uses a
\code{Uint8Array} validity mask and exposes the original length as
\code{length}.

R integer and logical \code{NA} values have validity zero and a placeholder
value of zero. Double \code{NA} is treated the same way, while ordinary
\code{NaN} remains a valid Float64 value. This preserves the R distinction
between \code{NA_real_} and \code{NaN}.

JavaScript can modify the contents of both typed arrays. Returning the branded
object to R reconstructs the original R vector type using the current values
and validity bytes. A nonzero validity byte is treated as valid. Removing the
brand by constructing a plain object returns the two buffers separately under
the ordinary object conversion rules.

The mask uses one byte per element. Use \code{js_typed_array()} when a mask is
not required. Factors and dates are rejected because their class semantics
cannot be represented by the numeric values and validity buffers alone.
}
\examples{
ctx <- JSContext$new(profile = "bare")
ctx$source(code = paste(
  "function fillMissing(x) {",
  "  x.values[1] = 20;",
  "  x.validity[1] = 1;",
  "  return x;",
  "}"
))
values <- js_masked_typed_array(c(10L, NA_integer_, 30L))
ctx$call("fillMissing", values)
}
