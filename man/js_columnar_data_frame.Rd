\name{js_columnar_data_frame}
\alias{js_columnar_data_frame}
\title{Transfer a data frame as an object of columns}
\usage{
js_columnar_data_frame(x, typed = TRUE)
}
\arguments{
  \item{x}{A data frame.}
  \item{typed}{Use typed arrays for eligible raw, integer, and double columns.}
}
\value{The data frame marked for column-oriented conversion.}
\description{
Transfers a data frame as one JavaScript object whose properties are columns.
This avoids creating one object per row. With \code{typed = TRUE}, raw,
integer, and double columns use contiguous typed arrays. Integer columns with
missing values, factors, and date-time columns use ordinary arrays. Double
\code{NA} and \code{NaN} values become JavaScript \code{NaN} in typed columns.
Explicit character row names are included as the \code{_row} column.
}
\examples{
ctx <- JSContext$new()
ctx$source(code = "function rows(x) { return x.value.length; }")
df <- data.frame(value = 1:4, label = letters[1:4])
ctx$call("rows", js_columnar_data_frame(df))
}
