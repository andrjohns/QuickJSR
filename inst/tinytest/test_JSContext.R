
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

# Test that R functions can be passed and evaluated in JS
jsc$source(code = "function fun_test(f, x, y) { return f(x, y); }")
expect_equal(jsc$call("fun_test", \(x, y){ x + y }, 1, 2), 3)

# Test that closures/captures work
a <- 3
expect_equal(jsc$call("fun_test", \(x, y){ (x + y) * a }, 1, 2), 9)

expect_equal(jsc$call("fun_test", \(x, y){ paste(x, y) }, "a", "b"), "a b")
