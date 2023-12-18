# QuickJSR 1.0.9

* Bundled QuickJS engine updated to the 2023-12-09 release:
  - added Object.hasOwn, {String|Array|TypedArray}.prototype.at,
    {Array|TypedArray}.prototype.findLast{Index}
  - BigInt support is enabled even if CONFIG_BIGNUM disabled
  - updated to Unicode 15.0.0
  - misc bug fixes
