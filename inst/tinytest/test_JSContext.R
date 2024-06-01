
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
expect_equal(jsc$call("fun_test", function(x, y){ x + y }, 1, 2), 3)

# Test that closures/captures work
a <- 3
expect_equal(jsc$call("fun_test", function(x, y){ (x + y) * a }, 1, 2), 9)
expect_equal(jsc$call("fun_test", function(x, y){ paste(x, y) }, "a", "b"), "a b")

# Test that R environments can be passed to JS and values accessed
jsc$source(code = "function env_test(env) { return env.a + env.b; }")
env <- new.env()
env$a <- 1
env$b <- 2
expect_equal(jsc$call("env_test", env), 3)

# Test JS functions can update values in R environments
jsc$source(code = "function env_update(env) { env.a = 10; env.b = 20; }")
jsc$call("env_update", env)
expect_equal(env$a, 10)
expect_equal(env$b, 20)

# Test that JS can call functions in R packages using the R object
jsc$source(code = 'function r_fun_test() { return R.package("QuickJSR")["get_tz_offset_seconds"]() }')
expect_equal(jsc$call("r_fun_test"), as.POSIXlt(Sys.time())$gmtoff)

# Fails on 3.6 CI, but can't be replicated locally
exit_if_not(R.version$major  > "3")

jsc$source(code = 'function r_fun_test1() { return R.package("base")["Sys.Date"]() }')
expect_equal(as.Date(jsc$call("r_fun_test1")), Sys.Date())
