ctx <- JSContext$new(profile = "bare")
ctx$source(code = paste(
  "function identity(x) { return x; }",
  "function constructors(x) {",
  "  return [x.values.constructor.name, x.validity.constructor.name];",
  "}",
  "function values(x) { return Array.from(x.values); }",
  "function validity(x) { return Array.from(x.validity); }",
  "function fillMissing(x) {",
  "  x.values[1] = 42;",
  "  x.validity[1] = 1;",
  "  return x;",
  "}",
  "function markMissing(x) { x.validity[0] = 0; return x; }",
  "function nested(x) { return { data: x }; }",
  "function detachValues(x) { x.values.buffer.transfer(); return x; }",
  "function detachValidity(x) { x.validity.buffer.transfer(); return x; }"
))

logical <- c(TRUE, NA, FALSE)
logical_masked <- js_masked_typed_array(logical)
expect_equal(
  ctx$call("constructors", logical_masked),
  c("Uint8Array", "Uint8Array")
)
expect_equal(
  ctx$call("values", logical_masked),
  c(1, 0, 0)
)
expect_equal(
  ctx$call("validity", logical_masked),
  c(1, 0, 1)
)
expect_identical(ctx$call("identity", logical_masked), logical)

integer <- c(-2L, NA_integer_, 3L)
integer_masked <- js_masked_typed_array(integer)
expect_equal(
  ctx$call("constructors", integer_masked),
  c("Int32Array", "Uint8Array")
)
expect_equal(
  ctx$call("values", integer_masked),
  c(-2, 0, 3)
)
expect_equal(
  ctx$call("validity", integer_masked),
  c(1, 0, 1)
)
expect_identical(ctx$call("identity", integer_masked), integer)

double <- c(1.5, NA_real_, NaN, Inf)
double_masked <- js_masked_typed_array(double)
double_result <- ctx$call("identity", double_masked)
expect_equal(double_result[c(1, 4)], double[c(1, 4)])
expect_true(is.na(double_result[2]))
expect_false(is.nan(double_result[2]))
expect_true(is.nan(double_result[3]))
expect_equal(
  ctx$call("constructors", double_masked),
  c("Float64Array", "Uint8Array")
)
expect_equal(
  ctx$call("validity", double_masked),
  c(1, 0, 1, 1)
)

expect_identical(
  ctx$call("fillMissing", js_masked_typed_array(integer)),
  c(-2L, 42L, 3L)
)
expect_identical(
  ctx$call("markMissing", js_masked_typed_array(c(1, 2, 3))),
  c(NA_real_, 2, 3)
)
expect_identical(
  ctx$call("nested", integer_masked),
  list(data = integer)
)

expect_identical(
  ctx$call("identity", js_masked_typed_array(logical())),
  logical()
)
expect_identical(
  ctx$call("identity", js_masked_typed_array(integer())),
  integer()
)
expect_identical(
  ctx$call("identity", js_masked_typed_array(double())),
  double()
)

masked_ref <- ctx$call_ref("identity", integer_masked)
values_view <- js_ref_get(masked_ref, "values") |> js_ref_to_altrep()
validity_view <- js_ref_get(masked_ref, "validity") |> js_ref_to_altrep()
expect_identical(values_view, c(-2L, 0L, 3L))
expect_identical(validity_view, as.raw(c(1, 0, 1)))
expect_identical(js_ref_to_r(masked_ref), integer)

expect_error(
  ctx$call("detachValues", js_masked_typed_array(integer)),
  "detached"
)
expect_error(
  ctx$call("detachValidity", js_masked_typed_array(integer)),
  "detached"
)

for (profile in c("host", "standard", "bare")) {
  profile_ctx <- JSContext$new(profile = profile)
  profile_ctx$source(code = "function identity(x) { return x; }")
  expect_identical(
    profile_ctx$call("identity", js_masked_typed_array(c(1L, NA_integer_))),
    c(1L, NA_integer_)
  )
}

expect_error(js_masked_typed_array(raw(2)), "logical, integer, or double")
expect_error(js_masked_typed_array(c("a", "b")), "logical, integer, or double")
expect_error(js_masked_typed_array(list(1, 2)), "logical, integer, or double")
expect_error(js_masked_typed_array(factor("a")), "factors and dates")
expect_error(js_masked_typed_array(as.Date("2026-08-24")), "factors and dates")
expect_error(
  js_masked_typed_array(as.POSIXct("2026-08-24", tz = "UTC")),
  "factors and dates"
)

forged <- structure(list(value = "invalid"), class = "quickjs_masked_typed_array")
expect_error(ctx$call("identity", forged), "require a logical, integer, or double")
