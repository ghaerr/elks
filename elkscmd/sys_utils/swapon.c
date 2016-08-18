/*
 * swapon.c
 *
 * Copyright (c) 2002 Harry Kalogirou
 * harkal@gmx.net
 *
 * This file may be distributed under the terms of the GNU General Public
 * License v2, or at your option any later version.
 */

#include <sys/stat.h>
#include <dirent.h>
#include <linuxmt/mem.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	int fd, sd;
	struct mem_swap_info si;
	struct stat statbuf;

	if (argc != 3){
		printf("syntax :\n   %s swap_device size\n", argv[0]);
		exit(1);
	}

	if ((fd = open("/dev/kmem", O_RDONLY)) < 0 || stat(argv[1], &statbuf) < 0) {
		perror(*argv);
		exit(1);
	}

	if (!S_ISBLK(statbuf.st_mode)){
		printf("%s not a block device\n", argv[1]);
		exit(1);
	}

	si.major = statbuf.st_rdev >> 8;
	si.minor = statbuf.st_rdev & 0xff;
	sscanf(argv[2], "%d", &si.size);

	printf("Adding swap device (%d, %d) size %d\n", si.major, si.minor, si.size);

	if (ioctl(fd, MEM_SETSWAP, &si)){
		perror(*argv);
		exit(1);
	}

	return 0;
}

