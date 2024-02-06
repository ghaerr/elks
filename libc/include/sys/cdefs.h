#ifndef __SYS_CDEFS_H
#define __SYS_CDEFS_H
/* compiler-specific definitions */

#if __STDC__

#define	__CONCAT(x,y)	x ## y
#define	__STRING(x)     #x
#define __ptr_t         void *

#else

#define	__CONCAT(x,y)	x/**/y
#define	__STRING(x)     "x"
#define __ptr_t         char *

#endif

#define __P(x) x        /* always ANSI C */

/* don't require <stdnoreturn.h> */
#if defined(__GNUC__)
#define noreturn        __attribute__((__noreturn__))
#else
#define noreturn
#endif

/* No C++ */
#define __BEGIN_DECLS
#define __END_DECLS

#endif
