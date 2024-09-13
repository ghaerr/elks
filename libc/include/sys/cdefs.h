#ifndef __SYS_CDEFS_H
#define __SYS_CDEFS_H
/* compiler-specific definitions for userspace */

#include <features.h>
#include __SYSARCHINC__(cdefs.h)

/* No C++ */
#define __BEGIN_DECLS
#define __END_DECLS

#endif
