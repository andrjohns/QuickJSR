# QuickJSR 1.1.0

* Bundled QuickJS engine updated to the 2024-01-13 release:
  - top-level-await support in modules
  - allow 'await' in the REPL
  - added Array.prototype.{with,toReversed,toSpliced,toSorted} and
  TypedArray.prototype.{with,toReversed,toSorted}
  - added String.prototype.isWellFormed and String.prototype.toWellFormed
  - added Object.groupBy and Map.groupBy
  - added Promise.withResolvers
  - class static block
  - 'in' operator support for private fields
  - optional chaining fixes
  - added RegExp 'd' flag
  - fixed RegExp zero length match logic
  - fixed RegExp case insensitive flag
  - added os.sleepAsync(), os.getpid() and os.now()
  - added cosmopolitan build
  - misc bug fixes

# QuickJSR 1.0.9

* Bundled QuickJS engine updated to the 2023-12-09 release:
  - added Object.hasOwn, {String|Array|TypedArray}.prototype.at,
    {Array|TypedArray}.prototype.findLast{Index}
  - BigInt support is enabled even if CONFIG_BIGNUM disabled
  - updated to Unicode 15.0.0
  - misc bug fixes
