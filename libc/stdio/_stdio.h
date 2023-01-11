#if !defined(_STDIO_H)
#define	_STDIO_H

#include <stdio.h>
#include <sys/linksym.h>

#if defined(__STDC__) && !defined(__FIRST_ARG_IN_AX__)
#	include <stdarg.h>
#	define va_strt      va_start
#else
#	include <varargs.h>
#	define va_strt(p,i) va_start(p)
#endif

extern FILE *__IO_list;		/* For fflush at exit */

#endif
