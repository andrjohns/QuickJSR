ctx <- JSContext$new(profile = "bare")

expect_equal(ctx$eval("globalThis.counter = 0; counter"), 0)
expect_equal(ctx$eval("counter += 1; counter"), 1)

script <- ctx$compile("counter += 1; counter")
expect_true(inherits(script, "JSCompiledScript"))
expect_true(inherits(script, "JSValueRef"))
expect_equal(ctx$get("counter"), 1)
expect_equal(js_script_run(script), 2)
expect_equal(js_script_run(script), 3)

array_script <- ctx$compile("[counter, counter + 1]")
array_ref <- js_script_run_ref(array_script)
expect_true(inherits(array_ref, "JSValueRef"))
expect_equal(js_ref_to_r(array_ref), c(3, 4))

function_ref <- ctx$eval_ref("value => value * 2")
expect_equal(js_ref_to_r(js_ref_call(function_ref, 4)), 8)
expect_equal(js_ref_to_r(js_ref_call(function_ref, 5)), 10)

expect_error(ctx$compile("function ("), "JavaScript Exception")
failing <- ctx$compile("throw new Error('compiled failure')")
expect_error(js_script_run(failing), "compiled failure")
expect_error(js_script_run(failing), "compiled failure")
expect_equal(js_script_run(script), 4)

other <- JSContext$new(profile = "bare")
other$source(code = "function identity(x) { return x; }")
expect_error(other$call("identity", script), "compiled scripts cannot")
expect_error(js_ref_to_r(script), "must be executed")
expect_error(js_script_run(ctx$eval_ref("1")), "Expected a JSCompiledScript")

orphan <- local({
  local_ctx <- JSContext$new(profile = "bare")
  local_ctx$compile("40 + 2")
})
gc()
expect_equal(js_script_run(orphan), 42)

expect_true(any(grepl(
  "compiled JavaScript script",
  capture.output(print(script))
)))
