#include <stdio.h>

void Rf_error(const char *, ...);
void Rprintf(const char *, ...);
void Rf_abort(void);
void Rf_exit(int status);
void Rf_putchar(int c);

#define abort Rf_abort
#define putchar Rf_putchar
#define _putchar Rf_putchar
#define exit Rf_exit
#define _exit Rf_exit
