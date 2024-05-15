
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

jsc2 <- JSContext$new()
jsc2$source(code = "function add_test_2(x, y) { return x + y + 2; }")
expect_equal(jsc2$call("add_test_2", 1, 2), 5)
