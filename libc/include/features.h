#ifndef __FEATURES_H
#define __FEATURES_H

/* Pick an OS sysinclude directory */
/* Use with #include __SYSINC__(errno.h) */

#define __SYSINC__(_h_file_) <linuxmt/_h_file_>
#define __SYSARCHINC__(_h_file_) <arch/_h_file_>

#include <sys/cdefs.h>

#endif
