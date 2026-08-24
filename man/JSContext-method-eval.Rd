\name{JSContext-method-eval}
\alias{JSContext-method-eval}
\alias{eval}
\title{Evaluate code in a persistent JavaScript context}
\usage{
eval(code)
}
\arguments{
  \item{code}{JavaScript source code to evaluate.}
}
\value{The evaluation result converted to R.}
\description{
Evaluates source code in an existing \code{JSContext}. Global state remains
available to later evaluations, calls, and compiled scripts. Use
\code{ctx$eval(code)} to invoke the method.
}
