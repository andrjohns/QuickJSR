\name{JSContext-reference-methods}
\alias{JSContext-method-call_ref}
\alias{JSContext-method-get_ref}
\alias{JSContext-method-eval_ref}
\alias{call_ref}
\alias{get_ref}
\alias{eval_ref}
\title{Create persistent JavaScript references}
\usage{
call_ref(function_name, ...)

get_ref(var_name)

eval_ref(code)
}
\arguments{
  \item{function_name}{The global function or dotted method path to call.}
  \item{...}{Arguments passed to the JavaScript function.}
  \item{var_name}{The global value or dotted property path to retrieve.}
  \item{code}{JavaScript source code to evaluate.}
}
\value{A \code{JSValueRef} owned by the context.}
\description{
Evaluate, retrieve, or call JavaScript while retaining the result inside the
context instead of eagerly converting it to R.
}
