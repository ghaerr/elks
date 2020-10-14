#ifndef __ARCH_8086_PARAM_H
#define __ARCH_8086_PARAM_H

#include <linuxmt/config.h>

/* We don't support the complex PLL time loops on Linux 8086
 * as 100Hz fixed is more than good enough for us.
 *
 * set to 100 HZ by Shani <kerr@wizard.net>, since that's what we're using!
 */
#ifdef CONFIG_ARCH_SIBO
#define HZ			30
#else
#define HZ			100
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
