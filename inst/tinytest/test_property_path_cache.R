ctx <- JSContext$new()
ctx$source(code = paste(
  "globalThis.root = {",
  "  branch: { value: 3, read() { return this.value; } }",
  "};"
))

value <- NULL
for (i in 1:100) value <- ctx$get("root.branch.value")
expect_equal(value, 3)

expect_equal(ctx$get("root.late"), NULL)
ctx$assign("root.late", 42)
expect_equal(ctx$get("root.late"), 42)

result <- NULL
for (i in 1:100) result <- ctx$call("root.branch.read")
expect_equal(result, 3)

ctx$source(code = paste(
  "globalThis.root = {",
  "  branch: { value: 9, read() { return this.value; } }",
  "};"
))
expect_equal(ctx$get("root.branch.value"), 9)
expect_equal(ctx$call("root.branch.read"), 9)

root_ref <- ctx$get_ref("root")
branch_ref <- js_ref_get(root_ref, "branch")
method_ref <- js_ref_get(branch_ref, "read")
expect_equal(js_ref_to_r(js_ref_call(method_ref)), 9)

other <- JSContext$new()
other$source(code = "globalThis.root = { branch: { value: 17 } };")
expect_equal(other$get("root.branch.value"), 17)
