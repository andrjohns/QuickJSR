
<!-- README.md is generated from README.Rmd. Please edit that file -->

# QuickJSR

<!-- badges: start -->

[![R-CMD-check](https://github.com/andrjohns/QuickJSR/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/andrjohns/QuickJSR/actions/workflows/R-CMD-check.yaml)
[![CRAN
status](https://www.r-pkg.org/badges/version/QuickJSR)](https://CRAN.R-project.org/package=QuickJSR)
[![:name status
badge](https://andrjohns.r-universe.dev/badges/:name)](https://andrjohns.r-universe.dev/)
[![QuickJSR status
badge](https://andrjohns.r-universe.dev/badges/QuickJSR)](https://andrjohns.r-universe.dev/QuickJSR)
<!-- badges: end -->

A portable, lightweight, zero-dependency JavaScript engine for R, using
[QuickJS](https://bellard.org/quickjs/).

Values and objects are directly passed between R and QuickJS, with no
need for serialization or deserialization. This both reduces overhead
allows for more complex data structures to be passed between R and
JavaScript - including functions.

## Installation

You can install the development version of QuickJSR from
[GitHub](https://github.com/) with:

``` r
# install.packages("remotes")
remotes::install_github("andrjohns/QuickJSR")
```

Or you can install pre-built binaries from R-Universe:

``` r
install.packages("QuickJSR", repos = c("https://andrjohns.r-universe.dev",
                                        "https://cran.r-project.org"))
```

## Usage

For standalone or simple JavaScript code, you can use the `qjs_eval()`
function:

``` r
library(QuickJSR)

qjs_eval("1 + 1")
#> [1] 2
```

``` r
qjs_eval("Math.random()")
#> [1] 0.7065871
```

For more complex interactions, you can create a QuickJS context and
evaluate code within that context:

``` r
ctx <- JSContext$new()
```

Use the `$source()` method to load JavaScript code into the context:

``` r
# Code can be provided as a string
ctx$source(code = "function add(a, b) { return a + b; }")

# Or read from a file
writeLines("function subtract(a, b) { return a - b; }", "subtract.js")
ctx$source(file = "subtract.js")
```

Then use the `$call()` method to call a specified function with
arguments:

``` r
ctx$call("add", 1, 2)
#> [1] 3
```

``` r
ctx$call("subtract", 5, 3)
#> [1] 2
```

You can also pass R functions to be evaluated using JavaScript
arguments:

``` r
ctx$source(code = "function callRFunction(f, x, y) { return f(x, y); }")
ctx$call("callRFunction", function(x, y) x + y, 1, 2)
#> [1] 3
```

``` r
ctx$call("callRFunction", function(x, y) paste0(x, ",", y), "a", "b")
#> [1] "a,b"
```
