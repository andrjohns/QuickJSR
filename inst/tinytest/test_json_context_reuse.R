expect_error(from_json("{"), "JavaScript Exception")
expect_equal(from_json("[1,2,3]"), 1:3)

result <- NULL
for (i in 1:100) {
  json <- to_json(list(value = i), auto_unbox = TRUE)
  result <- from_json(json)
}
expect_equal(result, list(value = 100))

gc()
expect_equal(from_json(to_json(c(1.5, 2.5))), c(1.5, 2.5))
