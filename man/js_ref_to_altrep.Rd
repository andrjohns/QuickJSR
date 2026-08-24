\name{js_ref_to_altrep}
\alias{js_ref_to_altrep}
\title{Create an R ALTREP view over a JavaScript typed array}
\usage{
js_ref_to_altrep(ref)
}
\arguments{
  \item{ref}{A \code{JSValueRef} containing a supported typed array.}
}
\value{
An ALTREP raw, integer, or double vector backed by the JavaScript ArrayBuffer.
}
\description{
Creates a zero-copy R view over the storage of a retained JavaScript typed
array. \code{Uint8Array} and \code{Uint8ClampedArray} produce raw vectors,
\code{Int32Array} produces an integer vector, and \code{Float64Array} produces
a double vector.
}
\details{
The backing ArrayBuffer is made permanently immutable before its address is
exposed to R. This affects every JavaScript alias of that buffer. Indexed
JavaScript writes are ignored, while mutating typed-array methods, DataView
writes, resizing, and transfer operations throw an error.

The ALTREP object retains the reference and its context while it reads the
JavaScript buffer. When R requests writable storage, the values are copied
into an ordinary R vector, the R modification affects only that copy, and the
JavaScript reference can be released.

Shared, detached, unsupported, and out-of-bounds typed-array buffers are
rejected. An \code{Int32Array} value of \code{-2147483648} is R's
\code{NA_integer_} sentinel. Float64 NaN payloads are preserved, including an
R \code{NA_real_} payload originally transferred to JavaScript.

ALTREP-aware R operations can read the buffer without copying. Other R code
may request writable or materialised storage and therefore trigger a copy.
}
\examples{
ctx <- JSContext$new(profile = "bare")
ref <- ctx$eval_ref("new Float64Array([1, 2, 3, 4])")
values <- js_ref_to_altrep(ref)
sum(values)
values[1]
}
