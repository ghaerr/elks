/* stdlib.h  <ndf@linux.mit.edu> */
#include <features.h>
#include <sys/types.h>

#ifndef __STDLIB_H
#define __STDLIB_H

#include <malloc.h>

/* Don't overwrite user definitions of NULL */
#ifndef NULL
#define NULL ((void *) 0)
#endif

/* For program termination */
#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

extern int rand __P ((void));
extern void srand __P ((unsigned int seed));

extern long strtol __P ((const char * nptr, char ** endptr, int base));
extern unsigned long strtoul __P ((const char * nptr,
				   char ** endptr, int base));
#ifndef __HAS_NO_FLOATS__
extern double strtod __P ((const char * nptr, char ** endptr));
extern double atof __P ((__const char *__nptr));
#endif

extern long int atol __P ((__const char *__nptr));
extern int atoi __P ((__const char *__nptr));

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

void abort (void);
int atexit (void (* function) ());
void exit (int status);
int system(const char *command);
void qsort(void *base, size_t nel, size_t width,
	int (*compar)(/*const void *, const void * */));

#ifndef __STRICT_ANSI__
void breakpoint ();
char *itoa (int);
char *ltoa (long);
#endif

char * ltostr (long val, int radix);
char * ultostr (unsigned long val, int radix);

#endif /* __STDLIB_H */
