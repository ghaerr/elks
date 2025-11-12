/* This file is part of the ELKS TCP/IP stack
 *
 * (C) 2001 Harry Kalogirou (harkal@rainbow.cs.unipi.gr)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linuxmt/mem.h>
#include "timer.h"

#define _MK_FP(seg,off) ((void __far *)((((unsigned long)(seg)) << 16) | ((unsigned int)(off))))

#if 0
timeq_t timer_get_time(void)
{
    struct timezone	tz;
    struct timeval 	tv;

    gettimeofday(&tv, &tz);

	/* return 1/16 second ticks, 1,000,000/16 = 62500*/
	/* return 1/10000000 second (us) ticks */
    return (tv.tv_sec << 4) | ((unsigned long)tv.tv_usec/62500L);
}
#endif

int timer_init(void)
{
    unsigned int kds, jaddr;
    int fd = open("/dev/kmem", O_RDONLY);

    if (fd < 0) {
	write(2, "No /dev/kmem\n", 13);
	return -1;
    }
    if ((ioctl(fd, MEM_GETDS, &kds) < 0) || (ioctl(fd, MEM_GETJIFFADDR, &jaddr)) < 0 )  {
	write(2, "kmem ioctl err\n", 15);
	return -1;
    }
    jp = _MK_FP(kds, jaddr);
    close(fd);
    return 0;
}
