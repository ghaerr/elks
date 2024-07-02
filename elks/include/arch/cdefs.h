#ifndef __ARCH_8086_CDEFS_H
#define __ARCH_8086_CDEFS_H
/* compiler-specific definitions for kernel and userspace */

#if __STDC__
#define	__CONCAT(x,y)	x ## y
#define	__STRING(x)     #x
#else
#define	__CONCAT(x,y)	x/**/y
#define	__STRING(x)     "x"
#endif

#define __P(x) x        /* always ANSI C */

/* don't require <stdnoreturn.h> */
#ifdef __GNUC__
#define noreturn        __attribute__((__noreturn__))
#define stdcall         __attribute__((__stdcall__))
#define restrict        __restrict
#define printfesque(n)  __attribute__((__format__(__gnu_printf__, n, n + 1)))
#define noinstrument    __attribute__((no_instrument_function))
#define __wcfar
#define __wcnear
#endif

#ifdef __WATCOMC__
#define noreturn        __declspec(noreturn)
#define stdcall         __stdcall
#define restrict        __restrict
#define printfesque(n)
#define noinstrument
#define __attribute__(n)
/* force __cdecl calling convention and no register saves in main() arc/argv */
#pragma aux main "*" modify [ bx cx dx si di ]
#define __wcfar         __far
#define __wcnear        __near
#endif


#endif
