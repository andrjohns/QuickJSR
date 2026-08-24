# Assign a value to a variable in the current context

Assign a value to a variable in the current context

## Usage

``` r
assign(var_name, value)
```

## Arguments

- var_name:

  The name of the variable to assign

- value:

  The value to assign to the variable

## Value

No return value, called for side effects

## Examples

``` r
if (FALSE) { # \dontrun{
ctx <- JSContext$new()
ctx$assign("a", 1)
ctx$get("a")
} # }
```
