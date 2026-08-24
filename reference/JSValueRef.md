# Persistent JavaScript value references

Retain values inside their JavaScript context so chained operations do
not convert intermediate values to R. References keep their context
alive and can only be passed back to that same context.

## Usage

``` r
js_ref_get(ref, name)

js_ref_call(ref, ...)

js_ref_to_r(ref)
```

## Arguments

- ref:

  A `JSValueRef` created by a `JSContext`.

- name:

  A property name or dotted property path.

- ...:

  Arguments passed to the referenced JavaScript function.

## Value

`js_ref_get()` and `js_ref_call()` return a `JSValueRef`.
`js_ref_to_r()` materialises the referenced value as an R object.

## Examples

``` r
ctx <- JSContext$new()
ref <- ctx$eval_ref("({ value: 2, add(x) { return this.value + x; } })")
result <- js_ref_call(js_ref_get(ref, "add"), 3)
js_ref_to_r(result)
#> [1] 5
```
