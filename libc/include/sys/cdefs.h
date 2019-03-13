
#ifndef __SYS_CDEFS_H
#define __SYS_CDEFS_H
#include <features.h>

#if __STDC__

#define	__CONCAT(x,y)	x ## y
#define	__STRING(x)	#x

/* This is not a typedef so `const __ptr_t' does the right thing.  */
#define __ptr_t void *
#ifndef __HAS_NO_FLOATS__
typedef long double __long_double_t;
#endif

#else

#define	__CONCAT(x,y)	x/**/y
#define	__STRING(x)	"x"

#define __ptr_t char *

#ifndef __HAS_NO_FLOATS__
typedef double __long_double_t;
#endif

#endif

/* No C++ */
#define __BEGIN_DECLS
#define __END_DECLS

/* GNUish things */
#define __CONSTVALUE
#define __CONSTVALUE2

#endif
