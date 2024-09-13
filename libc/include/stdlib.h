#ifndef __STDLIB_H
#define __STDLIB_H

/* stdlib.h  <ndf@linux.mit.edu> */
#include <features.h>
#include <sys/types.h>
#include <malloc.h>

/* Don't overwrite user definitions of NULL */
#ifndef NULL
#define NULL ((void *) 0)
#endif

/* For program termination */
#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

#define RAND_MAX	0x7fff
int rand(void);
void srand(unsigned int seed);

long strtol(const char *str, char **endptr, int base);
unsigned long strtoul(const char *str, char **endptr, int base);

#ifndef __HAS_NO_FLOATS__
double strtod(const char *nptr, char **endptr);
double atof(const char *str);
char *ecvt(double val, int ndig, int *pdecpt, int *psign);
char *fcvt(double val, int nfrac, int *pdecpt, int *psign);
void dtostr(double val, int style, int preci, char *buf);
#endif

long atol(const char *str);
int atoi(const char *str);

/* Returned by `div'.  */
typedef struct
  {
    int quot;			/* Quotient.  */
    int rem;			/* Remainder.  */
  } div_t;

/* Returned by `ldiv'.  */
typedef struct
  {
    long int quot;		/* Quotient.  */
    long int rem;		/* Remainder.  */
  } ldiv_t;


char *getenv(const char *name);
int putenv(char *string);
char *mktemp(char *template);

noreturn void abort(void);
int atexit (void (*function)(void));
noreturn void exit(int status);
noreturn void _exit(int status); /* syscall */
int system(const char *command);
void qsort(void *base, size_t nel, size_t width,
	int (*compar)(/*const void *, const void * */));
char *devname(dev_t dev, mode_t type);

#ifndef __STRICT_ANSI__
int (bsr)(int x);
char *itoa(int val);
char *uitoa(unsigned int val);
char *ltoa(long val);
char *ultoa(unsigned long val);
char *ltostr(long val, int radix);
char *lltostr(long long val, int radix);
char *ultostr(unsigned long val, int radix);
char *ulltostr(unsigned long long val, int radix);
#endif

#endif /* __STDLIB_H */
