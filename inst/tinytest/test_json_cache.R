# to_json / from_json share a lazily-created runtime and context; verify
# correctness holds across repeated reuse and error recovery.
expect_equal(to_json(1:3), "[1,2,3]")
expect_equal(to_json(c(1.5, 2.5)), "[1.5,2.5]")
expect_equal(to_json("x"), '["x"]')
expect_equal(to_json("x", auto_unbox = TRUE), '"x"')
expect_equal(to_json(list(a = 1, b = 2), auto_unbox = TRUE), '{"a":1,"b":2}')

# Empty containers
expect_equal(to_json(list()), "[]")
expect_equal(to_json(setNames(list(), character(0))), "{}")

expect_equal(from_json("[1,2,3]"), c(1, 2, 3))
expect_equal(from_json('{"x":1,"y":[1,2,3]}'), list(x = 1, y = c(1, 2, 3)))
expect_equal(length(from_json("[]")), 0L)
expect_equal(from_json("{}"), setNames(list(), character(0)))

# Repeated round-trips must stay correct with the shared context
for (i in 1:200) {
  val <- list(a = i * 1.0, b = paste0("s", i))
  expect_equal(from_json(to_json(val, auto_unbox = TRUE)), val)
}

# A failed conversion must not corrupt the shared context
expect_error(to_json(as.raw(1)))
expect_equal(to_json(1:3), "[1,2,3]")
expect_equal(from_json("[4,5,6]"), c(4, 5, 6))

# Invalid JSON errors cleanly
expect_error(from_json("{not valid"))
