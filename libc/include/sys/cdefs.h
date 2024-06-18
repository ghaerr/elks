#ifndef __SYS_CDEFS_H
#define __SYS_CDEFS_H
/* compiler-specific definitions for userspace */

#include <features.h>
#include __SYSARCHINC__(cdefs.h)

/* No C++ */
#define __BEGIN_DECLS
#define __END_DECLS

#ifdef __WATCOMC__
/*
 * Force no register saves in main(), saves space.
 */
#pragma aux main "*" modify [ bx cx dx si di ]
extern int main(int, char **);
#endif

#endif
