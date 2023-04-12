## R CMD check results

0 errors | 0 warnings | 1 note

* This is a new release.

## Addressing Maintainer Comments

> Please always write package names, software names and API (application 
> programming interface) names in single quotes in title and description.
> e.g: --> 'JavaScript', 'C'
> Please note that package names are case sensitive.

- All instances of JavaScript, C, and R placed in ''

> Please add \value to .Rd files regarding exported methods and explain 
> the functions results in the documentation. Please write about the 
> structure of the output (class) and also what the output means. (If a 
> function does not return a value, please document that too, e.g. 
> \value{No return value, called for side effects} or similar)

- Added `\value{}` entry for all exported functions/objects/methods 

> You write information messages to the console that cannot be easily 
suppressed.
> It is more R like to generate objects that can be used to extract the 
> information a user is interested in, and then print() that object. 
> Instead of print()/cat() rather use message()/warning() or 
> if(verbose)cat(..) (or maybe stop()) if you really have to write text to 
> the console. (except for print, summary, interactive functions) -> 
> R/flags.R

 - Updated default behaviour to return string
 - Added explanation to description for when to print to console
 
> Please always add all authors, contributors and copyright holders in the 
> Authors@R field with the appropriate roles.

 - Added Authors@R entries for `QuickJS` developers as `ctb` and `cph`
 - Both the package and the `QuickJS` engine use MIT license, so no re-licensing/modifications needed
 
