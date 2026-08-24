# Call a JS function in the current context

Call a specified function in the JavaScript context with the provided
arguments.

## Usage

``` r
call(function_name, ...)
```

## Arguments

- function_name:

  The function to be called

- ...:

  The arguments to be passed to the function

## Value

The result of calling the specified function

## Examples

``` r
if (FALSE) { # \dontrun{
ctx <- JSContext$new()
ctx$source(code = "function add(a, b) { return a + b; }")
ctx$call("add", 1, 2)
} # }
```
