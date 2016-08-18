/*
 * meminfo.c
 *
 * Copyright (c) 2002 Harry Kalogirou
 * harkal@gmx.net
 *
 * This file may be distributed under the terms of the GNU General Public
 * License v2, or at your option any later version.
 */

#include <stdio.h>
#include <linuxmt/mem.h>
#include <unistd.h>
#include <fcntl.h>

int main(argc, argv)
int argc;
char ** argv;
{
    struct mem_usage mu;
    int fd;

    if ((fd = open("/dev/kmem", O_RDONLY)) < 0) {
	perror("meminfo");
	exit(1);
    }
    if (ioctl(fd, MEM_GETUSAGE, &mu))
	perror("meminfo");

    printf("memory usage : %4dKB total, %4dKB used, %4dKB free\n",
		mu.used_memory + mu.free_memory, mu.used_memory,
		mu.free_memory);
    printf("swap usage   : %4dKB total, %4dKB used, %4dKB free\n",
		mu.used_swap + mu.free_swap, mu.used_swap, mu.free_swap);

    return 0;
}
