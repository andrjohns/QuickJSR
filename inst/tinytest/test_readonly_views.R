ctx <- JSContext$new(profile = "bare")
ctx$source(code = paste(
  "function identity(x) { return x; }",
  "function inspect(x) {",
  "  return [x.length, x[0], x[1], x[x.length - 1], x[1000]];",
  "}",
  "function materialize(x) { return x.toArray(); }",
  "function spread(x) { return [...x]; }",
  "function arrayFrom(x) { return Array.from(x); }",
  "function total(x) { return x.reduce((a, b) => a + b, 0); }",
  "function keys(x) { return Object.keys(x); }",
  "function assignIndex(x) { x[0] = 99; }",
  "function assignProperty(x) { x.extra = 99; }",
  "function deleteIndex(x) { delete x[0]; }",
  "function defineIndex(x) { Object.defineProperty(x, '0', { value: 99 }); }",
  "function dateValue(x) { return x[0].toISOString(); }"
))

integer_view <- js_readonly_view(c(1L, NA_integer_, 3L))
expect_equal(ctx$call("inspect", integer_view), c(3, 1, NA, 3, NA))
expect_equal(ctx$call("materialize", integer_view), c(1L, NA_integer_, 3L))
expect_equal(ctx$call("spread", integer_view), c(1L, NA_integer_, 3L))
expect_equal(ctx$call("arrayFrom", integer_view), c(1L, NA_integer_, 3L))
expect_equal(ctx$call("total", js_readonly_view(1:100)), 5050)
expect_equal(ctx$call("keys", integer_view), c("0", "1", "2"))
expect_identical(ctx$call("identity", integer_view), integer_view)

expect_equal(
  ctx$call("materialize", js_readonly_view(as.raw(c(0, 127, 255)))),
  c(0, 127, 255)
)
expect_equal(
  ctx$call("materialize", js_readonly_view(c(TRUE, NA, FALSE))),
  c(TRUE, NA, FALSE)
)
expect_equal(
  ctx$call("materialize", js_readonly_view(c(1.5, NA, NaN, Inf))),
  c(1.5, NA, NaN, Inf)
)
expect_equal(
  ctx$call("materialize", js_readonly_view(c("a", NA, "c"))),
  c("a", NA, "c")
)
expect_equal(
  ctx$call("materialize", js_readonly_view(factor(c("b", NA, "a")))),
  c("b", NA, "a")
)
expect_equal(
  ctx$call("dateValue", js_readonly_view(as.Date("2026-08-24"))),
  "2026-08-24T00:00:00.000Z"
)

original <- c(10L, 20L, 30L)
view <- js_readonly_view(original)
ctx$assign("copyOnWriteView", view)
original[1] <- 99L
expect_equal(ctx$eval("copyOnWriteView[0]"), 10)
expect_error(ctx$call("assignIndex", view), "read-only")
expect_error(ctx$call("assignProperty", view), "read-only")
expect_error(ctx$call("deleteIndex", view), "read-only")
expect_error(ctx$call("defineIndex", view), "not extensible")
expect_identical(original, c(99L, 20L, 30L))

unassigned <- c(4L, 5L, 6L)
unassigned_view <- js_readonly_view(unassigned)
unassigned[1] <- 40L
expect_equal(ctx$call("identity", unassigned_view)$value, c(4L, 5L, 6L))

for (profile in c("host", "standard", "bare")) {
  profile_ctx <- JSContext$new(profile = profile)
  profile_ctx$source(code = "function first(x) { return x[0]; }")
  expect_equal(profile_ctx$call("first", js_readonly_view(42)), 42)
}

expect_error(js_readonly_view(NULL), "raw, logical, integer, double, or character")
expect_error(js_readonly_view(list(1, 2)), "raw, logical, integer, double, or character")
expect_error(js_readonly_view(1 + 2i), "raw, logical, integer, double, or character")

retain_view <- local({
  value <- js_readonly_view(c("retained", "value"))
  ctx$assign("retainedView", value)
  value
})
rm(retain_view)
gc()
expect_equal(ctx$eval("retainedView[0]"), "retained")
expect_equal(ctx$eval("retainedView.toArray()"), c("retained", "value"))
