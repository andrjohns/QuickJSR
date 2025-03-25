#include <stdio.h>

void Rf_error(const char *, ...);
void Rprintf(const char *, ...);
void Rf_abort(void);
void Rf_exit(int status);
void Rf_putchar(int c);
FILE* get_con(int);

//extern FILE * R_Consolefile;
//extern FILE * R_Outputfile;

#define abort Rf_abort
#define putchar Rf_putchar
#define _putchar Rf_putchar
#define exit Rf_exit
#define _exit Rf_exit
#define printf Rprintf
#define stderr get_con(1)
#define stdout get_con(0)
// install.packages(getwd(),type="source",repos=NULL)
