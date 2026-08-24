profile_types <- function(profile) {
  ctx <- JSContext$new(profile = profile)
  ctx$source(code = "globalThis.types = [typeof std, typeof os, typeof R];")
  ctx$get("types")
}

expect_equal(profile_types("host"), c("object", "object", "object"))
expect_equal(profile_types("standard"), c("object", "object", "undefined"))
expect_equal(profile_types("bare"), c("undefined", "undefined", "undefined"))

host <- JSContext$new()
expect_equal(host$profile, "host")

standard <- JSContext$new(profile = "standard")
standard$source(code = "function add(a, b) { return a + b; }")
expect_equal(standard$call("add", 2, 3), 5)

bare <- JSContext$new(profile = "bare")
bare$source(code = "function add(a, b) { return a + b; }")
expect_equal(bare$call("add", 4, 5), 9)
bare$assign("value", c(1, 2, 3))
expect_equal(bare$get("value"), c(1, 2, 3))
bare$source(code = "function applyR(f, x) { return f(x); }")
expect_equal(bare$call("applyR", function(x) x * 2, 4), 8)

expect_error(JSContext$new(profile = "invalid"), "should be one of")
