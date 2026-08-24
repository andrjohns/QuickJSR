ctx <- JSContext$new(profile = "bare")
ctx$source(code = "function identity(x) { return x; }")

raw_ref <- ctx$eval_ref("new Uint8Array([0, 127, 255])")
raw_view <- js_ref_to_altrep(raw_ref)
expect_identical(raw_view, as.raw(c(0, 127, 255)))
expect_identical(typeof(raw_view), "raw")

clamped_ref <- ctx$eval_ref("new Uint8ClampedArray([-1, 128, 300])")
expect_identical(
  js_ref_to_altrep(clamped_ref),
  as.raw(c(0, 128, 255))
)

integer_ref <- ctx$eval_ref(
  "new Int32Array([-2147483648, -2, 0, 3, 2147483647])"
)
integer_view <- js_ref_to_altrep(integer_ref)
expect_identical(
  integer_view,
  c(NA_integer_, -2L, 0L, 3L, .Machine$integer.max)
)
expect_identical(typeof(integer_view), "integer")

double_ref <- ctx$eval_ref("new Float64Array([1.5, NaN, Infinity, -Infinity])")
double_view <- js_ref_to_altrep(double_ref)
expect_equal(double_view, c(1.5, NaN, Inf, -Inf))
expect_identical(typeof(double_view), "double")

missing_ref <- ctx$call_ref(
  "identity",
  js_typed_array(c(1, NA_real_, NaN))
)
missing_view <- js_ref_to_altrep(missing_ref)
expect_true(is.na(missing_view[2]))
expect_false(is.nan(missing_view[2]))
expect_true(is.nan(missing_view[3]))

offset_ref <- ctx$eval_ref(
  "new Float64Array(new Float64Array([1, 2, 3, 4]).buffer, 8, 2)"
)
expect_identical(js_ref_to_altrep(offset_ref), c(2, 3))
expect_identical(
  js_ref_to_altrep(ctx$eval_ref("new Uint8Array(0)")),
  raw()
)

ctx$source(code = paste(
  "globalThis.frozenValues = new Int32Array([1, 2, 3]);",
  "function mutateWithSet() { frozenValues.set([9], 0); }",
  "function mutateWithView() {",
  "  new DataView(frozenValues.buffer).setInt32(0, 9);",
  "}"
))
frozen_ref <- ctx$get_ref("frozenValues")
frozen_view <- js_ref_to_altrep(frozen_ref)
expect_true(ctx$eval("frozenValues.buffer.immutable"))
expect_equal(ctx$eval("frozenValues[0] = 9; frozenValues[0]"), 1)
expect_error(ctx$call("mutateWithSet"), "immutable")
expect_error(ctx$call("mutateWithView"), "immutable")
expect_error(ctx$eval("frozenValues.buffer.transfer()"), "immutable")
expect_identical(frozen_view, 1:3)

r_alias <- frozen_view
frozen_view[1] <- 20L
expect_identical(frozen_view, c(20L, 2L, 3L))
expect_identical(r_alias, 1:3)
expect_identical(js_ref_to_r(frozen_ref), 1:3)

serialized <- unserialize(serialize(r_alias, NULL))
expect_identical(serialized, 1:3)

orphan_view <- local({
  local_ctx <- JSContext$new(profile = "bare")
  local_ctx$eval_ref("new Float64Array([10, 20, 30])") |>
    js_ref_to_altrep()
})
gc()
gc()
expect_identical(orphan_view, c(10, 20, 30))

expect_error(
  js_ref_to_altrep(ctx$eval_ref("[1, 2, 3]")),
  "require a Uint8Array"
)
expect_error(
  js_ref_to_altrep(ctx$eval_ref("new Float32Array([1, 2])")),
  "require a Uint8Array"
)
expect_error(
  js_ref_to_altrep(ctx$eval_ref(
    "(() => { const x = new Uint8Array([1]); x.buffer.transfer(); return x; })()"
  )),
  "detached or out of bounds"
)
expect_error(
  js_ref_to_altrep(ctx$eval_ref(
    "new Uint8Array(new SharedArrayBuffer(3))"
  )),
  "regular ArrayBuffer"
)

script <- ctx$compile("new Float64Array([1, 2])")
expect_error(js_ref_to_altrep(script), "compiled scripts")
