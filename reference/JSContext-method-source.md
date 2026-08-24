# Evaluate JS string or file in the current context

Evaluate a provided JavaScript file or string within the initialised
context. Note that this method should only be used for initialising
functions or values within the context, no values are returned from this
function. See the `$call()` method for returning values.

## Usage

``` r
source(file = NULL, code = NULL)
```

## Arguments

- file:

  A path to the JavaScript file to load

- code:

  A single string of JavaScript to evaluate

## Value

No return value, called for side effects

## Examples

``` r
if (FALSE) { # \dontrun{
ctx <- JSContext$new()
ctx$source(file = "path/to/file.js")
ctx$source(code = "1 + 2")
} # }
```
