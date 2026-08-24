# Assess validity of JS code without evaluating

Checks whether JS code string is valid code in the current context

## Usage

``` r
validate(code_string)
```

## Arguments

- code_string:

  The JS code to check

## Value

A boolean indicating whether code is valid

## Examples

``` r
if (FALSE) { # \dontrun{
ctx <- JSContext$new()
ctx$validate("1 + 2")
} # }
```
