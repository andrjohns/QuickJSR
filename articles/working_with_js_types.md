# Working with R and JS Types

``` r

library(QuickJSR)
```

## Mappings and Conversions Between R and JS Types

`QuickJSR` uses the respective `C` APIs of `R` and `QuickJS` in order to
pass values between the two. This allows for increased efficiency in
passing and returning values (as no serialisation or de-serialisation is
required) and also allows for greater flexibility in working with R
closures, functions, and environments in JS code.

`QuickJSR` aims to broadly follow the conventions of `jsonlite` in terms
of how R types are converted to JS types and vice-versa.

### Primitive & Scalar Types

The following table outlines the basic mappings of primitive types
between R and JS types:

| R Type    | JS Type |
|-----------|---------|
| NULL      | null    |
| logical   | boolean |
| integer   | number  |
| double    | number  |
| character | string  |
| date      | date    |
| POSIXct   | date    |
| factor    | string  |

Note that the handling of `Date`/`POSIXct` types differs from
`jsonlite`, where they are converted to strings. In `QuickJSR`, they are
treated directly as `Date` objects in JS.

### Container Types

The following table outlines the basic mappings of container types
between R and JS types:

| R Type       | JS Type          |
|--------------|------------------|
| named list   | object           |
| unnamed list | array            |
| vector       | array            |
| array        | array            |
| matrix       | 2D number array  |
| data.frame   | array of objects |

Examples of the `matrix` and `data.frame` conversions are shown below:

``` r

m <- matrix(1:6, nrow = 2)
cat(to_json(m))
#> [[1,3,5],[2,4,6]]
```

``` r

df <- data.frame(a = 1:3, b = c("x", "y", "z"))
cat(to_json(df))
#> [{"a":1,"b":"x"},{"a":2,"b":"y"},{"a":3,"b":"z"}]
```

## Typed Arrays

Use
[`js_typed_array()`](https://andrjohns.github.io/QuickJSR/reference/js_typed_array.md)
when a raw, integer, or double vector should cross the boundary as
contiguous typed storage rather than as an ordinary JavaScript array.

``` r

ctx <- JSContext$new()
ctx$source(code = paste(
  "function typeName(x) { return x.constructor.name; }",
  "function total(x) { return x.reduce((a, b) => a + b, 0); }"
))

values <- js_typed_array(as.double(1:1000))
ctx$call("typeName", values)
#> [1] "Float64Array"
ctx$call("total", values)
#> [1] 500500
```

Raw vectors become `Uint8Array`, integer vectors become `Int32Array`,
and double vectors become `Float64Array`. Integer vectors containing
`NA` are rejected because `Int32Array` has no missing-value
representation. Double `NA` and `NaN` values are both treated as
JavaScript `NaN`. On return, the `Int32Array` value `-2147483648`
becomes R’s `NA_integer_` sentinel. JavaScript 64-bit BigInt arrays
return as doubles and can lose precision outside the exact integer range
of an R double.

JavaScript typed arrays are copied directly into matching R vector
storage on return. Use a persistent reference when the value should
remain in JavaScript across several operations, and materialise it only
when R needs the result.

## Missing Typed Values

Use
[`js_masked_typed_array()`](https://andrjohns.github.io/QuickJSR/reference/js_masked_typed_array.md)
when logical, integer, or double values need contiguous typed storage
without losing R missing values. JavaScript receives a branded object
containing `values`, `validity`, and `length`.

``` r

ctx$source(code = paste(
  "function fillMissing(x) {",
  "  x.values[1] = 20;",
  "  x.validity[1] = 1;",
  "  return x;",
  "}"
))

masked <- js_masked_typed_array(c(10L, NA_integer_, 30L))
ctx$call("fillMissing", masked)
#> [1] 10 20 30
```

Logical values use a `Uint8Array`, integers use an `Int32Array`, and
doubles use a `Float64Array`. The validity mask is a `Uint8Array` with
zero for missing and one for valid. A double `NA` is masked, while an
ordinary `NaN` remains a valid Float64 value.

JavaScript may modify both buffers. Returning the branded object
reconstructs the corresponding R vector using their current contents.
The mask costs one byte per element, so
[`js_typed_array()`](https://andrjohns.github.io/QuickJSR/reference/js_typed_array.md)
remains preferable when missing-value metadata is unnecessary. Factors
and dates are excluded because their class semantics require more than
numeric storage and a validity mask.

## Read-only R Vector Views

Use
[`js_readonly_view()`](https://andrjohns.github.io/QuickJSR/reference/js_readonly_view.md)
when JavaScript only needs to read an R vector. View creation wraps and
retains the R vector but does not copy or convert its elements.
Conversion happens one element at a time as JavaScript reads indexed
values.

``` r

ctx$source(code = paste(
  "function endpoints(x) { return [x[0], x[x.length - 1]]; }",
  "function viewTotal(x) { return x.reduce((a, b) => a + b, 0); }"
))

view <- js_readonly_view(as.double(1:1000))
ctx$call("endpoints", view)
#> [1]    1 1000
ctx$call("viewTotal", view)
#> [1] 500500
```

Views are array-like rather than typed arrays. They have `length`,
indexed access, iteration, and inherited `Array` methods.
`view.toArray()`, spread syntax, and `Array.from(view)` create an
ordinary mutable JavaScript array. Direct writes and deletes throw an
error.

The JavaScript object retains the wrapper and vector for its full
lifetime. Normal assignment in R still follows copy-on-write rules:
changing an R binding can create a new vector while an existing
JavaScript view continues to see the retained vector. Passing the view
back to R returns that retained object directly.

Use a view when JavaScript reads a small part of a large vector or when
avoiding the initial transfer is important. Use
[`js_typed_array()`](https://andrjohns.github.io/QuickJSR/reference/js_typed_array.md)
when JavaScript will scan the vector repeatedly, needs typed-array APIs,
or needs a mutable independent copy.

## Mutable R Vector Views

Use
[`js_mutable_view()`](https://andrjohns.github.io/QuickJSR/reference/js_mutable_view.md)
when JavaScript needs repeated, sparse mutation of a vector and R needs
to observe those changes without a full return conversion. The wrapper
owns an independent R copy, so the input binding is never changed.

``` r

ctx$source(code = paste(
  "function updateEndpoints(x) {",
  "  x[0] += 1;",
  "  x[x.length - 1] += 1;",
  "  return x;",
  "}"
))

original <- as.double(1:1000)
view <- js_mutable_view(original)
ctx$call("updateEndpoints", view)
#> $value
#>    [1]    2    2    3    4    5    6    7    8    9   10   11   12   13   14
#>   [15]   15   16   17   18   19   20   21   22   23   24   25   26   27   28
#>   [29]   29   30   31   32   33   34   35   36   37   38   39   40   41   42
#>   [43]   43   44   45   46   47   48   49   50   51   52   53   54   55   56
#>   [57]   57   58   59   60   61   62   63   64   65   66   67   68   69   70
#>   [71]   71   72   73   74   75   76   77   78   79   80   81   82   83   84
#>   [85]   85   86   87   88   89   90   91   92   93   94   95   96   97   98
#>   [99]   99  100  101  102  103  104  105  106  107  108  109  110  111  112
#>  [113]  113  114  115  116  117  118  119  120  121  122  123  124  125  126
#>  [127]  127  128  129  130  131  132  133  134  135  136  137  138  139  140
#>  [141]  141  142  143  144  145  146  147  148  149  150  151  152  153  154
#>  [155]  155  156  157  158  159  160  161  162  163  164  165  166  167  168
#>  [169]  169  170  171  172  173  174  175  176  177  178  179  180  181  182
#>  [183]  183  184  185  186  187  188  189  190  191  192  193  194  195  196
#>  [197]  197  198  199  200  201  202  203  204  205  206  207  208  209  210
#>  [211]  211  212  213  214  215  216  217  218  219  220  221  222  223  224
#>  [225]  225  226  227  228  229  230  231  232  233  234  235  236  237  238
#>  [239]  239  240  241  242  243  244  245  246  247  248  249  250  251  252
#>  [253]  253  254  255  256  257  258  259  260  261  262  263  264  265  266
#>  [267]  267  268  269  270  271  272  273  274  275  276  277  278  279  280
#>  [281]  281  282  283  284  285  286  287  288  289  290  291  292  293  294
#>  [295]  295  296  297  298  299  300  301  302  303  304  305  306  307  308
#>  [309]  309  310  311  312  313  314  315  316  317  318  319  320  321  322
#>  [323]  323  324  325  326  327  328  329  330  331  332  333  334  335  336
#>  [337]  337  338  339  340  341  342  343  344  345  346  347  348  349  350
#>  [351]  351  352  353  354  355  356  357  358  359  360  361  362  363  364
#>  [365]  365  366  367  368  369  370  371  372  373  374  375  376  377  378
#>  [379]  379  380  381  382  383  384  385  386  387  388  389  390  391  392
#>  [393]  393  394  395  396  397  398  399  400  401  402  403  404  405  406
#>  [407]  407  408  409  410  411  412  413  414  415  416  417  418  419  420
#>  [421]  421  422  423  424  425  426  427  428  429  430  431  432  433  434
#>  [435]  435  436  437  438  439  440  441  442  443  444  445  446  447  448
#>  [449]  449  450  451  452  453  454  455  456  457  458  459  460  461  462
#>  [463]  463  464  465  466  467  468  469  470  471  472  473  474  475  476
#>  [477]  477  478  479  480  481  482  483  484  485  486  487  488  489  490
#>  [491]  491  492  493  494  495  496  497  498  499  500  501  502  503  504
#>  [505]  505  506  507  508  509  510  511  512  513  514  515  516  517  518
#>  [519]  519  520  521  522  523  524  525  526  527  528  529  530  531  532
#>  [533]  533  534  535  536  537  538  539  540  541  542  543  544  545  546
#>  [547]  547  548  549  550  551  552  553  554  555  556  557  558  559  560
#>  [561]  561  562  563  564  565  566  567  568  569  570  571  572  573  574
#>  [575]  575  576  577  578  579  580  581  582  583  584  585  586  587  588
#>  [589]  589  590  591  592  593  594  595  596  597  598  599  600  601  602
#>  [603]  603  604  605  606  607  608  609  610  611  612  613  614  615  616
#>  [617]  617  618  619  620  621  622  623  624  625  626  627  628  629  630
#>  [631]  631  632  633  634  635  636  637  638  639  640  641  642  643  644
#>  [645]  645  646  647  648  649  650  651  652  653  654  655  656  657  658
#>  [659]  659  660  661  662  663  664  665  666  667  668  669  670  671  672
#>  [673]  673  674  675  676  677  678  679  680  681  682  683  684  685  686
#>  [687]  687  688  689  690  691  692  693  694  695  696  697  698  699  700
#>  [701]  701  702  703  704  705  706  707  708  709  710  711  712  713  714
#>  [715]  715  716  717  718  719  720  721  722  723  724  725  726  727  728
#>  [729]  729  730  731  732  733  734  735  736  737  738  739  740  741  742
#>  [743]  743  744  745  746  747  748  749  750  751  752  753  754  755  756
#>  [757]  757  758  759  760  761  762  763  764  765  766  767  768  769  770
#>  [771]  771  772  773  774  775  776  777  778  779  780  781  782  783  784
#>  [785]  785  786  787  788  789  790  791  792  793  794  795  796  797  798
#>  [799]  799  800  801  802  803  804  805  806  807  808  809  810  811  812
#>  [813]  813  814  815  816  817  818  819  820  821  822  823  824  825  826
#>  [827]  827  828  829  830  831  832  833  834  835  836  837  838  839  840
#>  [841]  841  842  843  844  845  846  847  848  849  850  851  852  853  854
#>  [855]  855  856  857  858  859  860  861  862  863  864  865  866  867  868
#>  [869]  869  870  871  872  873  874  875  876  877  878  879  880  881  882
#>  [883]  883  884  885  886  887  888  889  890  891  892  893  894  895  896
#>  [897]  897  898  899  900  901  902  903  904  905  906  907  908  909  910
#>  [911]  911  912  913  914  915  916  917  918  919  920  921  922  923  924
#>  [925]  925  926  927  928  929  930  931  932  933  934  935  936  937  938
#>  [939]  939  940  941  942  943  944  945  946  947  948  949  950  951  952
#>  [953]  953  954  955  956  957  958  959  960  961  962  963  964  965  966
#>  [967]  967  968  969  970  971  972  973  974  975  976  977  978  979  980
#>  [981]  981  982  983  984  985  986  987  988  989  990  991  992  993  994
#>  [995]  995  996  997  998  999 1001
#> 
#> attr(,"class")
#> [1] "quickjs_mutable_view"
view$value[c(1, length(view$value))]
#> [1]    2 1001
original[c(1, length(original))]
#> [1]    1 1000
```

Creating the wrapper copies the vector once. Passing it to JavaScript is
then constant time, and indexed reads and writes operate directly on its
retained R storage. Returning the JavaScript view recovers the same
wrapper without materialising an array.

Mutable views are fixed-length array-like objects rather than typed
arrays. Existing elements can be replaced directly or through methods
such as `reverse()`, [`sort()`](https://rdrr.io/r/base/sort.html),
`fill()`, and `copyWithin()`. Adding properties, growing the view, or
deleting elements throws an error. Writes are type checked, with `null`
and `undefined` representing R missing values except for raw vectors.
Only unclassed raw, logical, integer, double, and character vectors are
accepted.

If R modifies `view$value` after JavaScript retains the view, normal R
copy-on-write can detach the R binding from the retained storage.
Complete R updates before retaining a mutable view, then treat
JavaScript as its writer.

Use a mutable view for sparse or repeated mutation shared with R. Use
[`js_typed_array()`](https://andrjohns.github.io/QuickJSR/reference/js_typed_array.md)
when JavaScript needs typed-array APIs or will perform dense numeric
work entirely inside JavaScript.

## Column-oriented Data Frames

The compatibility conversion represents a data frame as an array of row
objects. Use
[`js_columnar_data_frame()`](https://andrjohns.github.io/QuickJSR/reference/js_columnar_data_frame.md)
when JavaScript can operate on whole columns instead.

``` r

df <- data.frame(
  value = as.double(1:1000),
  group = rep(c("a", "b"), 500)
)

ctx$source(code = paste(
  "function columnTypes(x) {",
  "  return [x.value.constructor.name, x.group.constructor.name];",
  "}"
))
ctx$call("columnTypes", js_columnar_data_frame(df))
#> [1] "Float64Array" "Array"
```

Eligible numeric columns become typed arrays by default. Integer columns
with missing values, factors, and date-time columns remain ordinary
arrays so their missing-value and class semantics are preserved. Set
`typed = FALSE` to use ordinary arrays for every column.

Note that the
[`to_json()`](https://andrjohns.github.io/QuickJSR/reference/to_json.md)
function operates by converting R objects to their JS equivalents, and
then calling `JSON.stringify()` on the result. This allows you to
explore how different types are being converted to JS.

### Functions and Closures

Functions and closures can be passed between R and JS code. In JS,
functions are represented as `Function` objects, and can be called
directly from JS code.

``` r

ctx <- JSContext$new()
ctx$source(code = "function callRFunction(f, x, y) { return f(x, y); }")

ctx$call("callRFunction", function(x, y) x + y, 1, 2)
#> [1] 3
ctx$call("callRFunction", function(x, y) paste0(x, ",", y), "a", "b")
#> [1] "a,b"
```

## Working with R Environments

R environments are represented in JS as a custom class: `REnv`. The
`REnv` class simply wraps the pointer to the R environment, and provides
methods for getting and setting values - this means that there is only a
‘cost’ for conversion when values or accessed or updated.

Environment values can be accessed using either `env.value` or
`env["value"]` syntax:

``` r

ctx$source(code = 'function env_test(env) { return env.a + env["b"]; }')
env <- new.env()
env$a <- 1
env$b <- 2
ctx$call("env_test", env)
#> [1] 3
```

Values in the environment can also be updated from JS code:

``` r

ctx$source(code = "function env_update(env) { env.a = 10; env.b = 20; }")
ctx$call("env_update", env)
#> NULL
env$a
#> [1] 10
env$b
#> [1] 20
```

## Accessing Package Namespaces & Functions

`QuickJSR` automatically adds a global object `R` to each context, which
can be used to access the namespaces of installed packages - and
subsequently extract and use functions and objects from them.

``` r

qjs_eval('R.package("base").getwd()')
#> [1] "/home/runner/work/QuickJSR/QuickJSR/vignettes"
```
