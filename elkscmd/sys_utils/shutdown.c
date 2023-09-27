/*
 * shutdown.c
 *
 * Derived from reboot.c by:
 * Copyright 2000 Alistair Riddoch
 * ajr@ecs.soton.ac.uk
 *
 * This file may be distributed under the terms of the GNU General Public
 * License v2, or at your option any later version.
 */

/*
 * This is a small version of shutdown for use in the ELKS project.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mount.h>

int main(int argc, char **argv)
{
	sync();
	if (umount("/")) {
		perror("umount");
		return 1;
	}
	sleep(3);
	if (reboot(0x1D1E,0xC0DE,0x6789)) {
		perror("shutdown");
		return 1;
	}
	return 0;
}
