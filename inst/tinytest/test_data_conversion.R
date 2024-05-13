# Check conversions from R types to JS types are consistent with jsonlite.
#  - The inputs are directly converted from R to JS types using the QuickJS API.
#  - The outputs are returned as JSON strings and parsed back to R using jsonlite.
#  - If the conversion is consistent, the output should be the same as the input.
expect_eq_jsonlite <- function(x) {
  expect_equal(qjs_passthrough(x), jsonlite::fromJSON(jsonlite::toJSON(x)))
}
expect_eq_jsonlite(1)
expect_eq_jsonlite(1:3)
expect_eq_jsonlite(c(1.5, 2.5))

expect_eq_jsonlite("a")
expect_eq_jsonlite(c("a", "b", "c"))

expect_eq_jsonlite(TRUE)
expect_eq_jsonlite(FALSE)
expect_eq_jsonlite(c(TRUE, FALSE))

expect_eq_jsonlite(list(1, 2, 3))
expect_eq_jsonlite(list(a = 1, b = 2, c = 3))
expect_eq_jsonlite(list(a = "d", b = "e", c = "f"))

expect_eq_jsonlite(list(c(1, 2), c(3, 4)))
expect_eq_jsonlite(list(list(1, 2), list(3, 4)))
expect_eq_jsonlite(list(list(a = 1, b = 2), list(c = 3, d = 4)))

expect_eq_jsonlite(list(c("e", "f"), c("g", "h")))
expect_eq_jsonlite(list(list("e", "f"), list("g", "h")))
expect_eq_jsonlite(list(list(a = "e", b = "f"), list(c = "g", d = "h")))
