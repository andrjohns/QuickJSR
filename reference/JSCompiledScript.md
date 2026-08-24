# Compile and repeatedly execute JavaScript

Compiled scripts avoid parsing source on every execution. The bytecode
remains owned by its original context and can be executed repeatedly.
The script keeps that context alive.

## Usage

``` r
js_script_run(script)

js_script_run_ref(script)
```

## Arguments

- script:

  A script created by `ctx$compile(code)`.

## Value

`js_script_run()` converts the result to R. `js_script_run_ref()`
returns a `JSValueRef` owned by the script's context.

## Details

Compilation does not execute the source. Compiled scripts cannot be
passed as ordinary JavaScript values. For parameterised work, evaluate a
JavaScript function with `ctx$eval_ref()` and call it repeatedly with
[`js_ref_call()`](https://andrjohns.github.io/QuickJSR/reference/JSValueRef.md).

## Examples

``` r
ctx <- JSContext$new(profile = "bare")
ctx$eval("globalThis.value = 0")
#> [1] 0
script <- ctx$compile("value += 1; value")
js_script_run(script)
#> [1] 1
js_script_run(script)
#> [1] 2
```
