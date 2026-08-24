# Expose an R vector as a read-only JavaScript view

Creates an array-like JavaScript object that reads directly from
retained R vector storage. Creating the view does not copy or convert
its elements. Elements are converted only when JavaScript accesses them.

## Usage

``` r
js_readonly_view(x)
```

## Arguments

- x:

  A raw, logical, integer, double, or character vector.

## Value

A lightweight wrapper around the input vector.

## Details

The view has `length`, indexed access, array iteration, and inherited
non-mutating `Array` methods. Use `view.toArray()` or `Array.from(view)`
when a mutable JavaScript array is required. Attempts to write or delete
indexed values throw an error.

The wrapper does not copy the vector. The view retains the wrapper and
vector until JavaScript releases them. Normal R assignment follows R's
copy-on-write rules, so modifying an R binding can detach it from the
storage retained by an existing view. Returning the view to R recovers
the wrapper without materialising a JavaScript array.

Missing values use the ordinary conversion rules: logical, integer,
double, and character `NA` become JavaScript `null`; `NaN` remains
`NaN`. Factors yield their labels, and dates yield JavaScript dates.

## Examples

``` r
ctx <- JSContext$new(profile = "bare")
ctx$source(code = paste(
  "function endpoints(x) { return [x[0], x[x.length - 1]]; }",
  "function total(x) { return x.reduce((a, b) => a + b, 0); }"
))
values <- js_readonly_view(as.double(1:1000))
ctx$call("endpoints", values)
#> [1]    1 1000
ctx$call("total", values)
#> [1] 500500
```
