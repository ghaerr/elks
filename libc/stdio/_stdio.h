#if !defined(_STDIO_H)
#define	_STDIO_H

#include <stdio.h>

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

#if defined(__cplusplus)
extern "C" {
#endif

void __io_init_vars(void);

#if defined(__cplusplus)
}
#endif

#endif
