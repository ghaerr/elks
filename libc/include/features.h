
#ifndef __FEATURES_H
#define __FEATURES_H

#ifdef __STDC__

#define __P(x) x
#define __const const

/* Not really ansi */
#ifdef __BCC__
#define const
#define volatile
#endif

#else /* K&R */

#define __P(x) ()
#define __const
#define const
#define volatile

#endif

/* Pick an OS sysinclude directory */
/* Use with #include __SYSINC__(errno.h) */

#define __SYSINC__(_h_file_) <linuxmt/_h_file_>
#define __SYSARCHINC__(_h_file_) <arch/_h_file_>

/* No C++ */
#define __BEGIN_DECLS
#define __END_DECLS

/* GNUish things */
#define __CONSTVALUE
#define __CONSTVALUE2

#include <sys/cdefs.h>

#endif
