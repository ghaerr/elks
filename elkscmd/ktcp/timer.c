/* This file is part of the ELKS TCP/IP stack
 *
 * (C) 2001 Harry Kalogirou (harkal@rainbow.cs.unipi.gr)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#define __LIBC__	/* to get types for memory.h FIXME */
//#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linuxmt/mem.h>
#include <linuxmt/types.h>
#include <linuxmt/memory.h>
#include <sys/ioctl.h>

#include "timer.h"

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
	perror("/dev/kmem");
	return -1;
    }
    if ((ioctl(fd, MEM_GETDS, &kds) < 0) || (ioctl(fd, MEM_GETJIFFADDR, &jaddr)) < 0 )  {
	perror("ktcp: ioctl error in /dev/kmem");
	return -1;
    }
    jp = _MK_FP(kds, jaddr);
    close(fd);
    return 0;
}
