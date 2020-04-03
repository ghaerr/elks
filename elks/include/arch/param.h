#ifndef __ARCH_8086_PARAM_H
#define __ARCH_8086_PARAM_H

#include <linuxmt/config.h>

#ifndef HZ

#ifdef CONFIG_ARCH_SIBO
#define HZ			30
#else
#define HZ			100
#endif

#endif

#define RUNNABLE_PAGESIZE	4096

#ifndef NGROUPS
#define NGROUPS			32
#endif

#ifndef NOGROUP
#define NOGROUP			(-1)
#endif

#define MAXHOSTNAMELEN		64	/* max length of hostname */

#endif
