#ifndef _ARCH_8086_PARAM_H
#define _ARCH_8086_PARAM_H

#include <linuxmt/config.h>


#ifndef HZ
#ifdef CONFIG_ARCH_SIBO

#define HZ 30

#else /* CONFIG_ARCH_SIBO */

#define HZ 100

#endif /* CONFIG_ARCH_SIBO */
#endif /* HZ */

#define EXEC_PAGESIZE	4096

#ifndef NGROUPS
#define NGROUPS		32
#endif

#ifndef NOGROUP
#define NOGROUP		(-1)
#endif

#define MAXHOSTNAMELEN	64	/* max length of hostname */

#endif /* _ARCH_8086_PARAM_H */
