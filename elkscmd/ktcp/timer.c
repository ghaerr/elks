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
#include <linuxmt/mem.h>    /* for MEM_GETDS */
#include <arch/irq.h>
#include "timer.h"

#define MK_FP(seg,off) \
    ((void __far *) ((((unsigned long)(seg)) << 16) | ((unsigned int)(off))))

timeq_t Now;
static volatile unsigned long __far *jp;

/*
 * Return time counted in 60ms (~1/16 sec) quantums.
 * (used to be the time type counted in 62.5ms)
 */
timeq_t get_time(void)
{
    timeq_t ticks;

    clr_irq();
    ticks = *jp;            /* 10ms intervals */
    set_irq();
    return ticks / 6;       /* 60ms intervals */
}

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
    jp = MK_FP(kds, jaddr);
    close(fd);
    return 0;
}
