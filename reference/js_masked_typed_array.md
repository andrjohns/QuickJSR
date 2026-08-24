# Transfer missing typed values with a validity mask

Transfers an R vector as a branded JavaScript object containing
contiguous `values` and `validity` typed arrays. The validity byte is
zero for a missing value and one otherwise.

## Usage

``` r
js_masked_typed_array(x)
```

## Arguments

- x:

  A logical, integer, or double vector without factor or date classes.

## Value

A lightweight wrapper around the input vector.

## Details

Logical values use a `Uint8Array`, integers use an `Int32Array`, and
doubles use a `Float64Array`. Every representation uses a `Uint8Array`
validity mask and exposes the original length as `length`.

R integer and logical `NA` values have validity zero and a placeholder
value of zero. Double `NA` is treated the same way, while ordinary `NaN`
remains a valid Float64 value. This preserves the R distinction between
`NA_real_` and `NaN`.

JavaScript can modify the contents of both typed arrays. Returning the
branded object to R reconstructs the original R vector type using the
current values and validity bytes. A nonzero validity byte is treated as
valid. Removing the brand by constructing a plain object returns the two
buffers separately under the ordinary object conversion rules.

The mask uses one byte per element. Use
[`js_typed_array()`](https://andrjohns.github.io/QuickJSR/reference/js_typed_array.md)
when a mask is not required. Factors and dates are rejected because
their class semantics cannot be represented by the numeric values and
validity buffers alone.

## Examples

``` r
ctx <- JSContext$new(profile = "bare")
ctx$source(code = paste(
  "function fillMissing(x) {",
  "  x.values[1] = 20;",
  "  x.validity[1] = 1;",
  "  return x;",
  "}"
))
values <- js_masked_typed_array(c(10L, NA_integer_, 30L))
ctx$call("fillMissing", values)
#> [1] 10 20 30
```
