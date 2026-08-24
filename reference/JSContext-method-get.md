# Get a variable from the current context

Get the value of a variable from the current context

## Usage

``` r
get(var_name)
```

## Arguments

- var_name:

  The name of the variable to retrieve

## Value

The value of the variable

## Examples

``` r
if (FALSE) { # \dontrun{
ctx <- JSContext$new()
ctx$source(code = "var a = 1;")
ctx$get("a")
} # }
```
