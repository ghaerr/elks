#ifndef _LINUXMT_TIMEX_H
#define _LINUXMT_TIMEX_H

/*
 *	We don't support the complex PLL time loops on Linux 8086
 *	100Hz fixed is more than good enough for us.
 */
 
/*#define HZ 18*/
/* set to 100 HZ by Shani <kerr@wizard.net>, since that's what we're using! */
#define HZ 100

#endif /* LINUXMT_TIMEX_H */
