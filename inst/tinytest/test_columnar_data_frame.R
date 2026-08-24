ctx <- JSContext$new()
ctx$source(code = paste(
  "function columnType(x, name) { return x[name].constructor.name; }",
  "function columnValue(x, name, index) { return x[name][index]; }"
))

df <- data.frame(
  integer = 1:3,
  double = c(1.5, 2.5, 3.5),
  character = c("a", "b", "c"),
  missing = c(1L, NA_integer_, 3L),
  factor = factor(c("a", "b", "a")),
  date = as.Date(c("2026-01-01", "2026-01-02", "2026-01-03"))
)
columnar <- js_columnar_data_frame(df)

expect_equal(ctx$call("columnType", columnar, "integer"), "Int32Array")
expect_equal(ctx$call("columnType", columnar, "double"), "Float64Array")
expect_equal(ctx$call("columnType", columnar, "character"), "Array")
expect_equal(ctx$call("columnType", columnar, "missing"), "Array")
expect_equal(ctx$call("columnType", columnar, "factor"), "Array")
expect_equal(ctx$call("columnType", columnar, "date"), "Array")
expect_equal(ctx$call("columnValue", columnar, "missing", 1), NULL)
expect_equal(ctx$call("columnValue", columnar, "factor", 1), "b")

plain <- js_columnar_data_frame(df, typed = FALSE)
expect_equal(ctx$call("columnType", plain, "integer"), "Array")
expect_equal(ctx$call("columnType", plain, "double"), "Array")

rownames(df) <- c("x", "y", "z")
named <- js_columnar_data_frame(df)
expect_equal(ctx$call("columnType", named, "_row"), "Array")
expect_equal(ctx$call("columnValue", named, "_row", 2), "z")

expect_error(js_columnar_data_frame(1:3), "x must be a data frame")
expect_error(js_columnar_data_frame(df, typed = NA), "typed must be TRUE or FALSE")
