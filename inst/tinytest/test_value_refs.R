ctx <- JSContext$new()
ctx$source(
  code = paste(
    "function identity(x) { return x; }",
    "function sum(x) { return x.reduce((a, b) => a + b, 0); }",
    "globalThis.counter = { value: 2, add(x) { return this.value + x; } };"
  )
)

array_ref <- ctx$eval_ref("[1, 2, 3, 4]")
expect_true(inherits(array_ref, "JSValueRef"))
expect_equal(ctx$call("sum", array_ref), 10)

identity_ref <- ctx$call_ref("identity", array_ref)
expect_true(inherits(identity_ref, "JSValueRef"))
expect_equal(js_ref_to_r(identity_ref), 1:4)

counter_ref <- ctx$get_ref("counter")
method_ref <- js_ref_get(counter_ref, "add")
expect_equal(js_ref_to_r(js_ref_call(method_ref, 5)), 7)

dotted_method_ref <- ctx$get_ref("counter.add")
expect_equal(js_ref_to_r(js_ref_call(dotted_method_ref, 8)), 10)

cycle_ref <- ctx$eval_ref("(() => { const x = {}; x.self = x; return x; })()")
self_ref <- js_ref_get(cycle_ref, "self")
expect_true(inherits(self_ref, "JSValueRef"))

other_ctx <- JSContext$new()
other_ctx$source(code = "function identity(x) { return x; }")
expect_error(other_ctx$call("identity", array_ref), "different context")

orphan_ref <- local({
  local_ctx <- JSContext$new()
  local_ctx$eval_ref("({ answer: 42 })")
})
gc()
gc()
expect_equal(js_ref_to_r(orphan_ref), list(answer = 42))

ctx$source(code = "function failRef() { throw new Error('reference failed'); }")
expect_error(ctx$call_ref("failRef"), "reference failed")
expect_equal(ctx$call("sum", array_ref), 10)

expect_true(any(grepl("QuickJSR JavaScript reference", capture.output(print(array_ref)))))
