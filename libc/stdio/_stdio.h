#if !defined(_STDIO_H)
#define	_STDIO_H

#include <stdio.h>

#if defined(__STDC__) && !defined(__FIRST_ARG_IN_AX__)
#	include <stdarg.h>
#	define va_strt      va_start
#else
#	include <varargs.h>
#	define va_strt(p,i) va_start(p)
#endif

#ifdef __AS386_16__
#define Inline_init
#endif

#ifdef __AS386_32__
#define Inline_init
#endif

#ifndef Inline_init
#define Inline_init __io_init_vars()
#endif

extern FILE *__IO_list;		/* For fflush at exit */
extern int (*__fp_print)();

#if defined(__cplusplus)
extern "C" {
#endif

void __io_init_vars(void);
void __fp_print_func(double * pval, int style, int preci, char * ptmp);

#if defined(__cplusplus)
}
#endif

#endif
