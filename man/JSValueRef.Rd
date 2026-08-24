\name{JSValueRef}
\alias{JSValueRef}
\alias{js_ref_get}
\alias{js_ref_call}
\alias{js_ref_to_r}
\title{Persistent JavaScript value references}
\usage{
js_ref_get(ref, name)

js_ref_call(ref, ...)

js_ref_to_r(ref)
}
\arguments{
  \item{ref}{A \code{JSValueRef} created by a \code{JSContext}.}
  \item{name}{A property name or dotted property path.}
  \item{...}{Arguments passed to the referenced JavaScript function.}
}
\value{
\code{js_ref_get()} and \code{js_ref_call()} return a \code{JSValueRef}.
\code{js_ref_to_r()} materialises the referenced value as an R object.
}
\description{
Retain values inside their JavaScript context so chained operations do not
convert intermediate values to R. References keep their context alive and can
only be passed back to that same context.
}
\examples{
ctx <- JSContext$new()
ref <- ctx$eval_ref("({ value: 2, add(x) { return this.value + x; } })")
result <- js_ref_call(js_ref_get(ref, "add"), 3)
js_ref_to_r(result)
}
