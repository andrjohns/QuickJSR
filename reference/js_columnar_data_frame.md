# Transfer a data frame as an object of columns

Transfers a data frame as one JavaScript object whose properties are
columns. This avoids creating one object per row. With `typed = TRUE`,
raw, integer, and double columns use contiguous typed arrays. Integer
columns with missing values, factors, and date-time columns use ordinary
arrays. Double `NA` and `NaN` values become JavaScript `NaN` in typed
columns. Explicit character row names are included as the `_row` column.

## Usage

``` r
js_columnar_data_frame(x, typed = TRUE)
```

## Arguments

- x:

  A data frame.

- typed:

  Use typed arrays for eligible raw, integer, and double columns.

## Value

The data frame marked for column-oriented conversion.

## Examples

``` r
ctx <- JSContext$new()
ctx$source(code = "function rows(x) { return x.value.length; }")
df <- data.frame(value = 1:4, label = letters[1:4])
ctx$call("rows", js_columnar_data_frame(df))
#> [1] 4
```
