# Create an R ALTREP view over a JavaScript typed array

Creates a zero-copy R view over the storage of a retained JavaScript
typed array. `Uint8Array` and `Uint8ClampedArray` produce raw vectors,
`Int32Array` produces an integer vector, and `Float64Array` produces a
double vector.

## Usage

``` r
js_ref_to_altrep(ref)
```

## Arguments

- ref:

  A `JSValueRef` containing a supported typed array.

## Value

An ALTREP raw, integer, or double vector backed by the JavaScript
ArrayBuffer.

## Details

The backing ArrayBuffer is made permanently immutable before its address
is exposed to R. This affects every JavaScript alias of that buffer.
Indexed JavaScript writes are ignored, while mutating typed-array
methods, DataView writes, resizing, and transfer operations throw an
error.

The ALTREP object retains the reference and its context while it reads
the JavaScript buffer. When R requests writable storage, the values are
copied into an ordinary R vector, the R modification affects only that
copy, and the JavaScript reference can be released.

Shared, detached, unsupported, and out-of-bounds typed-array buffers are
rejected. An `Int32Array` value of `-2147483648` is R's `NA_integer_`
sentinel. Float64 NaN payloads are preserved, including an R `NA_real_`
payload originally transferred to JavaScript.

ALTREP-aware R operations can read the buffer without copying. Other R
code may request writable or materialised storage and therefore trigger
a copy.

## Examples

``` r
ctx <- JSContext$new(profile = "bare")
ref <- ctx$eval_ref("new Float64Array([1, 2, 3, 4])")
values <- js_ref_to_altrep(ref)
sum(values)
#> [1] 10
values[1]
#> [1] 1
```
