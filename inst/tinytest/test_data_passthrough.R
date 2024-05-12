expect_equal(qjs_passthrough(1), 1)
expect_equal(qjs_passthrough(c(1, 2, 3)), 1:3)
expect_equal(qjs_passthrough(c(1.5, 2.5)), c(1.5, 2.5))

expect_equal(qjs_passthrough("a"), "a")
expect_equal(qjs_passthrough(c("a", "b", "c")), c("a", "b", "c"))

expect_equal(qjs_passthrough(TRUE), TRUE)
expect_equal(qjs_passthrough(FALSE), FALSE)
expect_equal(qjs_passthrough(c(TRUE, FALSE)), c(TRUE, FALSE))
