test_that("JSContext methods work", {
  jsc <- JSContext$new()
  jsc$source(code = "function add_test(x, y) { return x + y; }")
  expect_true(jsc$validate("add_test"))

  expect_equal(jsc$call("add_test", 1, 2), 3)
  expect_equal(jsc$call("add_test", 1, "a"), "1a")

  js_file <- tempfile(fileext = ".js")
  writeLines("function mult_test(x, y) { return x * y; }", con = js_file)
  jsc$source(file = js_file)

  expect_true(jsc$validate("mult_test"))
  expect_equal(jsc$call("mult_test", 1, 2), 2)
  expect_equal(jsc$call("mult_test", 10, 15), 150)
})
