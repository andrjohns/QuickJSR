# qjs_eval

Evaluate a single Javascript expression.

## Usage

``` r
qjs_eval(eval_string)
```

## Arguments

- eval_string:

  A single string of the expression to evaluate

## Value

The result of the provided expression

## Examples

``` r
# Return the sum of two numbers:
qjs_eval("1 + 2")
#> [1] 3

# Concatenate strings:
qjs_eval("'1' + '2'")
#> [1] "12"

# Create lists from objects:
qjs_eval("var t = {'a' : 1, 'b' : 2}; t")
#> $a
#> [1] 1
#> 
#> $b
#> [1] 2
#> 
```
