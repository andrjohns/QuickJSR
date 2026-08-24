ctx <- JSContext$new()
ctx$source(code = paste(
  "function identity(x) { return x; }",
  "function constructorName(x) { return x.constructor.name; }",
  "function total(x) { return x.reduce((a, b) => a + b, 0); }"
))

raw_value <- as.raw(c(0, 1, 127, 255))
integer_value <- c(-10L, 0L, 10L, .Machine$integer.max)
double_value <- c(-1.5, 0, 2.5, NA_real_, NaN, Inf)

expect_equal(ctx$call("constructorName", js_typed_array(raw_value)), "Uint8Array")
expect_equal(ctx$call("constructorName", js_typed_array(integer_value)), "Int32Array")
expect_equal(ctx$call("constructorName", js_typed_array(double_value)), "Float64Array")

expect_identical(ctx$call("identity", js_typed_array(raw_value)), raw_value)
expect_identical(ctx$call("identity", js_typed_array(integer_value)), integer_value)

double_result <- ctx$call("identity", js_typed_array(double_value))
expect_equal(double_result[1:3], double_value[1:3])
expect_true(is.na(double_result[4]))
expect_true(is.nan(double_result[5]))
expect_equal(double_result[6], Inf)

expect_equal(ctx$call("total", js_typed_array(as.double(1:100))), 5050)
expect_identical(ctx$call("identity", js_typed_array(raw())), raw())
expect_identical(ctx$call("identity", js_typed_array(integer())), integer())
expect_identical(ctx$call("identity", js_typed_array(double())), double())

expect_identical(qjs_eval("new Uint8Array([0, 255])"), as.raw(c(0, 255)))
expect_identical(qjs_eval("new Uint8ClampedArray([-1, 300])"), as.raw(c(0, 255)))
expect_identical(qjs_eval("new Int8Array([-128, 127])"), c(-128L, 127L))
expect_identical(qjs_eval("new Int16Array([-32768, 32767])"), c(-32768L, 32767L))
expect_identical(qjs_eval("new Uint16Array([0, 65535])"), c(0L, 65535L))
expect_identical(qjs_eval("new Int32Array([-2147483648, 2147483647])"), c(NA_integer_, 2147483647L))
expect_equal(qjs_eval("new Uint32Array([0, 4294967295])"), c(0, 4294967295))
expect_equal(qjs_eval("new Float32Array([1.5, -2.25])"), c(1.5, -2.25))
expect_equal(qjs_eval("new Float64Array([1.5, -2.25])"), c(1.5, -2.25))
expect_equal(qjs_eval("new Float16Array([1.5, -2.25])"), c(1.5, -2.25))
expect_equal(qjs_eval("new BigInt64Array([1n, -2n])"), c(1, -2))
expect_equal(qjs_eval("new BigUint64Array([1n, 2n])"), c(1, 2))

expect_equal(
  qjs_eval("new Float64Array(new Float64Array([1, 2, 3, 4]).buffer, 8, 2)"),
  c(2, 3)
)

expect_error(js_typed_array(c(TRUE, FALSE)), "raw, integer, or double")
expect_error(js_typed_array(c(1L, NA_integer_)), "cannot contain NA")
forged <- structure(c(1L, NA_integer_), class = "quickjs_typed_array")
expect_error(ctx$call("identity", forged), "cannot contain NA")

expect_equal(qjs_eval("[1, 2, 3, 4]"), c(1, 2, 3, 4))
expect_equal(qjs_eval("[1, null, 3]"), c(1, NA, 3))
