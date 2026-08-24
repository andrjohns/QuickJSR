# Create persistent JavaScript references

Evaluate, retrieve, or call JavaScript while retaining the result inside
the context instead of eagerly converting it to R.

## Usage

``` r
call_ref(function_name, ...)

get_ref(var_name)

eval_ref(code)
```

## Arguments

- function_name:

  The global function or dotted method path to call.

- ...:

  Arguments passed to the JavaScript function.

- var_name:

  The global value or dotted property path to retrieve.

- code:

  JavaScript source code to evaluate.

## Value

A `JSValueRef` owned by the context.
