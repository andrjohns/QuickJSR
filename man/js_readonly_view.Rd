\name{js_readonly_view}
\alias{js_readonly_view}
\title{Expose an R vector as a read-only JavaScript view}
\usage{
js_readonly_view(x)
}
\arguments{
  \item{x}{A raw, logical, integer, double, or character vector.}
}
\value{A lightweight wrapper around the input vector.}
\description{
Creates an array-like JavaScript object that reads directly from retained R
vector storage. Creating the view does not copy or convert its elements.
Elements are converted only when JavaScript accesses them.
}
\details{
The view has \code{length}, indexed access, array iteration, and inherited
non-mutating \code{Array} methods. Use \code{view.toArray()} or
\code{Array.from(view)} when a mutable JavaScript array is required. Attempts
to write or delete indexed values throw an error.

The wrapper does not copy the vector. The view retains the wrapper and vector
until JavaScript releases them. Normal R assignment follows R's copy-on-write
rules, so modifying an R binding can detach it from the storage retained by an
existing view. Returning the view to R recovers the wrapper without
materialising a JavaScript array.

Missing values use the ordinary conversion rules: logical, integer, double,
and character \code{NA} become JavaScript \code{null}; \code{NaN} remains
\code{NaN}. Factors yield their labels, and dates yield JavaScript dates.
}
\examples{
ctx <- JSContext$new(profile = "bare")
ctx$source(code = paste(
  "function endpoints(x) { return [x[0], x[x.length - 1]]; }",
  "function total(x) { return x.reduce((a, b) => a + b, 0); }"
))
values <- js_readonly_view(as.double(1:1000))
ctx$call("endpoints", values)
ctx$call("total", values)
}
