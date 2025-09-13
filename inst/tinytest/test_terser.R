code <- "function add(first, second) { return first + second; }"
expect_equal(terser(code)$code, "function add(n,d){return n+d}");

code <- "console.log(add(1 + 2, 3 + 4));"
options <- list(
  toplevel = TRUE,
  compress = list(
    passes = 2,
    global_defs = list(
      "@console.log" = "alert"
    )
  ),
  format = list(
    preamble = "/* minified */"
  )
)

expect_equal(
  terser(code, options)$code,
  "/* minified */\nalert(add(3,7));"
)
