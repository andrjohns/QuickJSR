# Expose owned R vector storage as a mutable JavaScript view

Creates a fixed-length array-like JavaScript object whose indexed writes
update R-owned vector storage. The wrapper makes one copy when it is
created; passing it to JavaScript does not copy or convert the complete
vector.

## Usage

``` r
js_mutable_view(x)
```

## Arguments

- x:

  An unclassed raw, logical, integer, double, or character vector.

## Value

A mutable-view wrapper containing an independent copy in `value`.

## Details

JavaScript reads convert elements on demand and writes update
`view$value`. The caller's original vector is never modified. Multiple
JavaScript views of the same wrapper share its owned storage.

The view has `length`, indexed access, enumeration, iteration, inherited
`Array` methods, and `toArray()`. Its length cannot change, properties
cannot be added, and elements cannot be deleted. Methods such as
`reverse`, `sort`, `fill`, and `copyWithin` work because they only
replace existing elements.

Assignments are type checked. JavaScript `null` and `undefined` write R
missing values except for raw vectors. JavaScript `NaN` remains an
ordinary R `NaN`. Factors, dates, and other classed vectors are
rejected.

Normal R copy-on-write rules still apply. Replacing or modifying
`view$value` in R after JavaScript retains the view can detach the R
binding from the retained storage.

## Examples

``` r
ctx <- JSContext$new(profile = "bare")
ctx$source(code = "function increment(x) { x[0] += 1; return x; }")
values <- js_mutable_view(c(1, 2, 3))
ctx$call("increment", values)
#> $value
#> [1] 2 2 3
#> 
#> attr(,"class")
#> [1] "quickjs_mutable_view"
values$value
#> [1] 2 2 3
```
