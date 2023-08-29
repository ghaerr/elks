/*
 * reboot.c
 *
 * Copyright 2000 Alistair Riddoch
 * ajr@ecs.soton.ac.uk
 *
 * This file may be distributed under the terms of the GNU General Public
 * License v2, or at your option any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/mount.h>
#include <sys/select.h>

int main(int argc, char **argv)
{
	sync();
	if (umount("/") < 0) {
		/* -f forces reboot even if mount fails */
		if (argc < 2 || argv[1][0] != '-' || argv[1][1] != 'f')	 {
			perror("umount");
			return 1;
		}
	}
	sleep(3);
	if (reboot(0x1D1E,0xC0DE,0x0123)) {
		perror("reboot");
		return 1;
	}
	return 0;
}
