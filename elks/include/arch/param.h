#ifndef __ARCH_8086_PARAM_H
#define __ARCH_8086_PARAM_H

#include <linuxmt/config.h>

/* We don't support the complex PLL time loops on Linux 8086
 * as 100Hz fixed is more than good enough for us.
 *
 * set to 100 HZ by Shani <kerr@wizard.net>, since that's what we're using!
 */
#define HZ			100

#endif
