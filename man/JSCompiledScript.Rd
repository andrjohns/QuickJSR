\name{JSCompiledScript}
\alias{JSCompiledScript}
\alias{js_script_run}
\alias{js_script_run_ref}
\title{Compile and repeatedly execute JavaScript}
\usage{
js_script_run(script)

js_script_run_ref(script)
}
\arguments{
  \item{script}{A script created by \code{ctx$compile(code)}.}
}
\value{
\code{js_script_run()} converts the result to R. \code{js_script_run_ref()}
returns a \code{JSValueRef} owned by the script's context.
}
\description{
Compiled scripts avoid parsing source on every execution. The bytecode remains
owned by its original context and can be executed repeatedly. The script keeps
that context alive.
}
\details{
Compilation does not execute the source. Compiled scripts cannot be passed as
ordinary JavaScript values. For parameterised work, evaluate a JavaScript
function with \code{ctx$eval_ref()} and call it repeatedly with
\code{js_ref_call()}.
}
\examples{
ctx <- JSContext$new(profile = "bare")
ctx$eval("globalThis.value = 0")
script <- ctx$compile("value += 1; value")
js_script_run(script)
js_script_run(script)
}
