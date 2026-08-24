\name{js_mutable_view}
\alias{js_mutable_view}
\title{Expose owned R vector storage as a mutable JavaScript view}
\usage{
js_mutable_view(x)
}
\arguments{
  \item{x}{An unclassed raw, logical, integer, double, or character vector.}
}
\value{A mutable-view wrapper containing an independent copy in \code{value}.}
\description{
Creates a fixed-length array-like JavaScript object whose indexed writes update
R-owned vector storage. The wrapper makes one copy when it is created; passing
it to JavaScript does not copy or convert the complete vector.
}
\details{
JavaScript reads convert elements on demand and writes update
\code{view$value}. The caller's original vector is never modified. Multiple
JavaScript views of the same wrapper share its owned storage.

The view has \code{length}, indexed access, enumeration, iteration, inherited
\code{Array} methods, and \code{toArray()}. Its length cannot change, properties
cannot be added, and elements cannot be deleted. Methods such as \code{reverse},
\code{sort}, \code{fill}, and \code{copyWithin} work because they only replace
existing elements.

Assignments are type checked. JavaScript \code{null} and \code{undefined}
write R missing values except for raw vectors. JavaScript \code{NaN} remains an
ordinary R \code{NaN}. Factors, dates, and other classed vectors are rejected.

Normal R copy-on-write rules still apply. Replacing or modifying
\code{view$value} in R after JavaScript retains the view can detach the R
binding from the retained storage.
}
\examples{
ctx <- JSContext$new(profile = "bare")
ctx$source(code = "function increment(x) { x[0] += 1; return x; }")
values <- js_mutable_view(c(1, 2, 3))
ctx$call("increment", values)
values$value
}
