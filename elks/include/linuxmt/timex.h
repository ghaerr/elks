#ifndef __LINUXMT_TIMEX_H__
#define __LINUXMT_TIMEX_H__

#include <linuxmt/config.h>

/*
 *	We don't support the complex PLL time loops on Linux 8086
 *	100Hz fixed is more than good enough for us.
 */

/*#define HZ 18*/
/* set to 100 HZ by Shani <kerr@wizard.net>, since that's what we're using! */
#ifdef CONFIG_ARCH_SIBO

#define HZ 30

#else

#define HZ 100

#endif
#endif
