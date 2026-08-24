# Create a JavaScript context

Creates a persistent QuickJS runtime and context.

## Usage

``` r
new_JSContext(
  stack_size = NULL,
  profile = c("host", "standard", "bare")
)
```

## Arguments

- stack_size:

  Optional maximum JavaScript stack size in bytes.

- profile:

  Context facilities to install. See Details.

## Value

A persistent `JSContext`.

## Details

Create contexts through `JSContext$new()`.

The `host` profile preserves the complete API with `std`, `os`, console
helpers, and the global `R` bridge. The `standard` profile installs
`std`, `os`, and console helpers without the global `R` bridge. The
`bare` profile installs only the core JavaScript runtime. R values can
still be passed through context methods in every profile.

## Examples

``` r
ctx <- JSContext$new(profile = "bare")
ctx$source(code = "function add(a, b) { return a + b; }")
ctx$call("add", 2, 3)
#> [1] 5
```
