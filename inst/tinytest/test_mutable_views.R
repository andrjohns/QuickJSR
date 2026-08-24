ctx <- JSContext$new(profile = "bare")
ctx$source(code = paste(
  "function identity(x) { return x; }",
  "function mutateInteger(x) { x[0] = 40; x[1] = null; return x; }",
  "function mutateRaw(x) { x[0] = 255; return x; }",
  "function mutateLogical(x) { x[0] = false; x[1] = null; return x; }",
  "function mutateDouble(x) { x[0] = Infinity; x[1] = NaN; x[2] = null; return x; }",
  "function mutateCharacter(x) { x[0] = 'changed'; x[1] = null; return x; }",
  "function reverse(x) { x.reverse(); return x.toArray(); }",
  "function keys(x) { return Object.keys(x); }",
  "function assignOutside(x) { x[x.length] = 1; }",
  "function assignProperty(x) { x.extra = 1; }",
  "function removeFirst(x) { delete x[0]; }",
  "function badRaw(x) { x[0] = 256; }",
  "function badLogical(x) { x[0] = 1; }",
  "function badInteger(x) { x[0] = 1.5; }",
  "function badDouble(x) { x[0] = '1'; }",
  "function badCharacter(x) { x[0] = 1; }"
))

original <- 1:3
integer_view <- js_mutable_view(original)
expect_identical(integer_view$value, 1:3)
expect_identical(ctx$call("mutateInteger", integer_view), integer_view)
expect_identical(integer_view$value, c(40L, NA_integer_, 3L))
expect_identical(original, 1:3)

raw_view <- js_mutable_view(as.raw(c(0, 1)))
ctx$call("mutateRaw", raw_view)
expect_identical(raw_view$value, as.raw(c(255, 1)))

logical_view <- js_mutable_view(c(TRUE, FALSE))
ctx$call("mutateLogical", logical_view)
expect_identical(logical_view$value, c(FALSE, NA))

double_view <- js_mutable_view(c(1, 2, 3))
ctx$call("mutateDouble", double_view)
expect_true(is.infinite(double_view$value[1]))
expect_true(is.nan(double_view$value[2]))
expect_true(is.na(double_view$value[3]))
expect_false(is.nan(double_view$value[3]))

character_view <- js_mutable_view(c("a", "b"))
ctx$call("mutateCharacter", character_view)
expect_identical(character_view$value, c("changed", NA_character_))

reverse_view <- js_mutable_view(1:4)
expect_equal(ctx$call("reverse", reverse_view), 4:1)
expect_identical(reverse_view$value, 4:1)
expect_identical(ctx$call("keys", reverse_view), as.character(0:3))

ctx$assign("retainedMutable", integer_view)
ctx$eval("retainedMutable[2] = 80")
expect_identical(integer_view$value, c(40L, NA_integer_, 80L))
expect_true(ctx$eval("Object.getOwnPropertyDescriptor(retainedMutable, '0').writable"))
expect_false(ctx$eval("Object.isExtensible(retainedMutable)"))

expect_error(ctx$call("assignOutside", integer_view), "out of bounds")
expect_error(ctx$call("assignProperty", integer_view), "out of bounds")
expect_error(ctx$call("removeFirst", integer_view), "fixed length")
expect_error(ctx$call("badRaw", raw_view), "integers from 0 to 255")
expect_error(ctx$call("badLogical", logical_view), "boolean or null")
expect_error(ctx$call("badInteger", integer_view), "R integers or null")
expect_error(ctx$call("badDouble", double_view), "numbers or null")
expect_error(ctx$call("badCharacter", character_view), "strings or null")
expect_error(ctx$eval("retainedMutable.push(1)"), "out of bounds")
expect_error(ctx$eval("Object.defineProperty(retainedMutable, '0', {value: 1})"), "not extensible")

for (profile in c("host", "standard", "bare")) {
  profile_ctx <- JSContext$new(profile = profile)
  profile_ctx$source(code = "function setFirst(x) { x[0] = 7; return x; }")
  profile_view <- js_mutable_view(c(1L, 2L))
  expect_identical(profile_ctx$call("setFirst", profile_view), profile_view)
  expect_identical(profile_view$value, c(7L, 2L))
}

expect_error(js_mutable_view(NULL), "unclassed raw, logical, integer, double, or character")
expect_error(js_mutable_view(list(1, 2)), "unclassed raw, logical, integer, double, or character")
expect_error(js_mutable_view(1 + 2i), "unclassed raw, logical, integer, double, or character")
expect_error(js_mutable_view(factor("a")), "unclassed")
expect_error(js_mutable_view(as.Date("2026-08-24")), "unclassed")

forged <- structure(list(value = factor("a")), class = "quickjs_mutable_view")
expect_error(ctx$assign("forgedMutable", forged), "unclassed vector")
forged <- structure(list(value = 1:3, extra = 4L), class = "quickjs_mutable_view")
expect_error(ctx$assign("forgedMutable", forged), "invalid R vector view")

retained <- local({
  value <- js_mutable_view(c("retained", "value"))
  ctx$assign("retainedMutableLifetime", value)
  value
})
rm(retained)
gc()
ctx$eval("retainedMutableLifetime[1] = 'changed'")
expect_identical(
  ctx$eval("retainedMutableLifetime")$value,
  c("retained", "changed")
)
