# Check conversions from R types to JS types are consistent with jsonlite.
#  - The inputs are directly converted from R to JS types using the QuickJS API.
#  - The outputs are returned as JSON strings
#  - If the conversion is consistent, the output should match jsonlite's conversion
expect_eq_jsonlite <- function(x) {
  expect_equal(to_json(x), as.character(jsonlite::toJSON(x)))
}
expect_equal(to_json(1), "[1]")
expect_equal(to_json(1:3), "[1,2,3]")
expect_equal(to_json(c(1.5, 2.5)), "[1.5,2.5]")

expect_equal(to_json("a"), "[\"a\"]")
expect_equal(to_json(c("a", "b", "c")), "[\"a\",\"b\",\"c\"]")

expect_equal(to_json(TRUE), "[true]")
expect_equal(to_json(FALSE), "[false]")
expect_equal(to_json(c(TRUE, FALSE)), "[true,false]")

expect_equal(to_json(list(1, 2, 3)), "[[1],[2],[3]]")
expect_equal(to_json(list(a = 1, b = 2, c = 3)),
              "{\"a\":[1],\"b\":[2],\"c\":[3]}")
expect_equal(to_json(list(a = "d", b = "e", c = "f")),
              "{\"a\":[\"d\"],\"b\":[\"e\"],\"c\":[\"f\"]}")

expect_equal(to_json(list(c(1, 2), c(3, 4))), "[[1,2],[3,4]]")
expect_equal(to_json(list(list(1, 2), list(3, 4))), "[[[1],[2]],[[3],[4]]]")
expect_equal(to_json(list(list(a = 1, b = 2), list(c = 3, d = 4))),
              "[{\"a\":[1],\"b\":[2]},{\"c\":[3],\"d\":[4]}]")

expect_equal(to_json(list(c("e", "f"), c("g", "h"))),
              "[[\"e\",\"f\"],[\"g\",\"h\"]]")
expect_equal(to_json(list(list("e", "f"), list("g", "h"))),
              "[[[\"e\"],[\"f\"]],[[\"g\"],[\"h\"]]]")
expect_equal(to_json(list(list(a = "e", b = "f"), list(c = "g", d = "h"))),
              "[{\"a\":[\"e\"],\"b\":[\"f\"]},{\"c\":[\"g\"],\"d\":[\"h\"]}]")

# Test that the full round-trip conversion is consistent.
expect_eq_jsonlite_full <- function(x) {
  x_json <- jsonlite::toJSON(x)
  expect_equal(from_json(x_json), jsonlite::fromJSON(x_json))
}
expect_eq_jsonlite_full(1)
expect_eq_jsonlite_full(1:3)
expect_eq_jsonlite_full(c(1.5, 2.5))

expect_eq_jsonlite_full("a")
expect_eq_jsonlite_full(c("a", "b", "c"))

expect_eq_jsonlite_full(TRUE)
expect_eq_jsonlite_full(FALSE)
expect_eq_jsonlite_full(c(TRUE, FALSE))
