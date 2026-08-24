# Persistent JavaScript References

QuickJSR normally converts JavaScript results to R objects immediately.
This is convenient for scalar results and final outputs, but repeated
conversion is expensive when large values pass through several
JavaScript operations.

A `JSValueRef` keeps a value inside its JavaScript context. Property
access and function calls can then operate on the original value without
converting it to R. Conversion only occurs when
[`js_ref_to_r()`](https://andrjohns.github.io/QuickJSR/reference/JSValueRef.md)
is called.

## Context profiles

`JSContext$new()` uses the `"host"` profile by default. It includes the
QuickJS `std` and `os` modules, console helpers, and the global `R`
bridge. Use `"standard"` when JavaScript needs the standard modules but
not the global R bridge, or `"bare"` for only the core JavaScript
runtime.

``` r

bare_ctx <- JSContext$new(profile = "bare")
bare_ctx$source(code = "function add(a, b) { return a + b; }")
bare_ctx$call("add", 2, 3)
#> [1] 5
```

All profiles support ordinary R-to-JavaScript value transfer. References
remain owned by the context that created them.

## Persistent and compiled evaluation

Use `ctx$eval()` when source should execute in an existing context and
return an R value. Unlike
[`qjs_eval()`](https://andrjohns.github.io/QuickJSR/reference/qjs_eval.md),
it preserves global state between calls.

``` r

bare_ctx$eval("globalThis.count = 0; count")
#> [1] 0
bare_ctx$eval("count += 1; count")
#> [1] 1
```

Use `ctx$compile()` when the same global script will execute repeatedly.
Compilation parses the source without running it.
[`js_script_run()`](https://andrjohns.github.io/QuickJSR/reference/JSCompiledScript.md)
converts each result to R, while
[`js_script_run_ref()`](https://andrjohns.github.io/QuickJSR/reference/JSCompiledScript.md)
retains the result in JavaScript.

``` r

increment <- bare_ctx$compile("count += 1; count")
js_script_run(increment)
#> [1] 2
js_script_run(increment)
#> [1] 3

array_script <- bare_ctx$compile("[count, count + 1]")
array_result <- js_script_run_ref(array_script)
js_ref_to_r(array_result)
#> [1] 3 4
```

Compiled scripts take no arguments. For parameterised repeated work,
evaluate a function once with
[`eval_ref()`](https://andrjohns.github.io/QuickJSR/reference/JSContext-reference-methods.md)
and invoke it with
[`js_ref_call()`](https://andrjohns.github.io/QuickJSR/reference/JSValueRef.md).

``` r

double <- bare_ctx$eval_ref("value => value * 2")
js_ref_to_r(js_ref_call(double, 21))
#> [1] 42
```

## Creating references

A context can create references by evaluating code, retrieving a global
value, or calling a function.

``` r

ctx <- JSContext$new()
ctx$source(code = paste(
  "function identity(x) { return x; }",
  "function doubleValues(x) { return x.map(value => value * 2); }",
  "function sumValues(x) { return x.reduce((a, b) => a + b, 0); }"
))

evaluated <- ctx$eval_ref("[1, 2, 3, 4]")
retrieved <- ctx$get_ref("Math")
called <- ctx$call_ref("doubleValues", evaluated)

evaluated
#> <QuickJSR JavaScript reference>
called
#> <QuickJSR JavaScript reference>
```

The three context methods are:

| Method | Operation |
|----|----|
| `ctx$eval_ref(code)` | Evaluate code and retain its result |
| `ctx$get_ref(name)` | Retrieve a global value or dotted property path |
| `ctx$call_ref(name, ...)` | Call a global function or dotted method and retain its result |

## Passing references back to JavaScript

A reference from a context can be passed to ordinary or
reference-returning calls on that same context. The referenced value is
reused directly.

``` r

ctx$call("sumValues", evaluated)
#> [1] 10

doubled <- ctx$call_ref("doubleValues", evaluated)
ctx$call("sumValues", doubled)
#> [1] 20
```

This is the main performance benefit: intermediate arrays and objects do
not cross the R and JavaScript boundary.

## Property access and callable references

Use
[`js_ref_get()`](https://andrjohns.github.io/QuickJSR/reference/JSValueRef.md)
to retain a property and
[`js_ref_call()`](https://andrjohns.github.io/QuickJSR/reference/JSValueRef.md)
to call a retained function. Both return another `JSValueRef`.

``` r

pipeline <- ctx$eval_ref(
  "({
    values: [1, 2, 3],
    scale(multiplier) {
      this.values = this.values.map(value => value * multiplier);
      return this;
    },
    total() {
      return this.values.reduce((a, b) => a + b, 0);
    }
  })"
)

scale <- js_ref_get(pipeline, "scale")
scaled <- js_ref_call(scale, 10)
total <- js_ref_call(js_ref_get(scaled, "total"))

js_ref_to_r(total)
#> [1] 60
```

The reference returned for `scale` retains `pipeline` as its method
receiver, so JavaScript evaluates the call with the correct `this`
value. Dotted context paths retain receivers in the same way.

``` r

ctx$source(code = "globalThis.counter = { value: 5, read() { return this.value; } }")
read <- ctx$get_ref("counter.read")
js_ref_to_r(js_ref_call(read))
#> [1] 5
```

## Materialising a value in R

Call
[`js_ref_to_r()`](https://andrjohns.github.io/QuickJSR/reference/JSValueRef.md)
when the value is needed in R.

``` r

js_ref_to_r(doubled)
#> [1] 2 4 6 8
```

Materialisation uses the normal QuickJSR conversion rules. Keeping it
explicit makes the location and cost of conversion visible.

## Zero-copy typed-array views in R

Use
[`js_ref_to_altrep()`](https://andrjohns.github.io/QuickJSR/reference/js_ref_to_altrep.md)
when R only needs to read a retained `Uint8Array`, `Uint8ClampedArray`,
`Int32Array`, or `Float64Array`. It returns an ALTREP raw, integer, or
double vector backed directly by the JavaScript ArrayBuffer.

``` r

typed <- ctx$eval_ref("new Float64Array([1, 2, 3, 4])")
r_view <- js_ref_to_altrep(typed)
sum(r_view)
#> [1] 10
r_view[1]
#> [1] 1
```

Creating the view permanently makes the backing ArrayBuffer immutable.
This freezes every JavaScript alias of the same buffer and prevents
transfer, resize, typed-array mutation methods, and DataView writes.
Direct indexed writes are ignored by the JavaScript engine.

R reads can use the buffer without copying. If R requests writable
storage, the ALTREP object copies the values into R-owned memory and
releases its need to retain the JavaScript context. Subsequent R
modification affects only the R copy.

This API is appropriate when the JavaScript buffer is complete and ready
for read-only consumption. Use
[`js_ref_to_r()`](https://andrjohns.github.io/QuickJSR/reference/JSValueRef.md)
when JavaScript must retain mutable storage or when the typed-array
element type requires conversion.

## Context ownership

Every reference belongs to one context. Passing it to another context
raises an error because QuickJS values cannot move between runtimes.

``` r

other_ctx <- JSContext$new()
other_ctx$source(code = "function identity(x) { return x; }")
other_ctx$call("identity", evaluated)
#> Error:
#> ! JSValueRef belongs to a different context
```

A reference keeps its original context alive. It remains valid even if
the context object is no longer otherwise reachable from R.

``` r

detached <- local({
  local_ctx <- JSContext$new()
  local_ctx$eval_ref("({ answer: 42 })")
})

gc()
#>           used (Mb) gc trigger (Mb) max used (Mb)
#> Ncells  779982 41.7    1438204 76.9  1438204 76.9
#> Vcells 1398227 10.7    8388608 64.0  2459368 18.8
js_ref_to_r(detached)
#> $answer
#> [1] 42
```

## Cyclic and non-convertible values

References can retain cyclic objects, functions, promises, maps, sets,
and class instances without flattening them into R data structures.

``` r

cycle <- ctx$eval_ref(
  "(() => {
    const value = { name: 'cycle' };
    value.self = value;
    return value;
  })()"
)

self <- js_ref_get(cycle, "self")
self
#> <QuickJSR JavaScript reference>
```

Do not materialise a cyclic graph unless the conversion policy supports
its shape. Continue operating on it through references instead.

## Choosing between values and references

Use ordinary
[`call()`](https://andrjohns.github.io/QuickJSR/reference/JSContext-method-call.md)
and
[`get()`](https://andrjohns.github.io/QuickJSR/reference/JSContext-method-get.md)
for scalars, small final results, and values that are immediately needed
in R. Use references when values are large, when several JavaScript
operations are chained, or when JavaScript identity and structure must
be preserved.

A typical high-performance workflow is:

1.  Transfer or create data once with
    [`eval_ref()`](https://andrjohns.github.io/QuickJSR/reference/JSContext-reference-methods.md),
    [`get_ref()`](https://andrjohns.github.io/QuickJSR/reference/JSContext-reference-methods.md),
    or
    [`call_ref()`](https://andrjohns.github.io/QuickJSR/reference/JSContext-reference-methods.md).
2.  Chain JavaScript work with
    [`js_ref_get()`](https://andrjohns.github.io/QuickJSR/reference/JSValueRef.md)
    and
    [`js_ref_call()`](https://andrjohns.github.io/QuickJSR/reference/JSValueRef.md).
3.  Return small scalar summaries through ordinary
    [`call()`](https://andrjohns.github.io/QuickJSR/reference/JSContext-method-call.md)
    where practical.
4.  Materialise large results once with
    [`js_ref_to_r()`](https://andrjohns.github.io/QuickJSR/reference/JSValueRef.md).
