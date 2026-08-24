# Evaluate code in a persistent JavaScript context

Evaluates source code in an existing `JSContext`. Global state remains
available to later evaluations, calls, and compiled scripts. Use
`ctx$eval(code)` to invoke the method.

## Usage

``` r
eval(code)
```

## Arguments

- code:

  JavaScript source code to evaluate.

## Value

The evaluation result converted to R.
