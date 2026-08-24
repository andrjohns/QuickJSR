# Transfer an R vector as a JavaScript typed array

Marks a vector for contiguous transfer to JavaScript. Raw vectors become
`Uint8Array`, integer vectors become `Int32Array`, and double vectors
become `Float64Array`. Integer vectors containing `NA` are rejected.
Double `NA` and `NaN` values both become JavaScript `NaN`. On return,
the JavaScript `Int32Array` value `-2147483648` becomes R's
`NA_integer_` sentinel.

## Usage

``` r
js_typed_array(x)
```

## Arguments

- x:

  A raw, integer, or double vector.

## Value

The input vector marked for typed-array conversion.

## Examples

``` r
ctx <- JSContext$new()
ctx$source(code = "function total(x) { return x.reduce((a, b) => a + b, 0); }")
ctx$call("total", js_typed_array(as.double(1:4)))
#> [1] 10
```
