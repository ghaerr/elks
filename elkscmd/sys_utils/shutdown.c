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
#include <linuxmt/limits.h>

int try_unmount(dev_t dev)
{
	struct statfs statfs;
	if (ustatfs(dev, &statfs, UF_NOFREESPACE) < 0) {
		return 0;
	}
	if (umount(statfs.f_mntonname)) {
		perror("umount");
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int i, ret;
	sync();
	for (i = NR_SUPER - 1; i >= 0; --i) {
		ret = try_unmount(i);
		if (ret) return 1;
	}
	sleep(3);
	if (reboot(0x1D1E,0xC0DE,0x6789)) {
		perror("shutdown");
		return 1;
	}
	return 0;
}
