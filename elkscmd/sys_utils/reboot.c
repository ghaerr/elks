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
	int force = 1;
	if (argc < 2 || argv[1][0] != '-' || argv[1][1] != 'f')
		force = 0;

	sync();
	for (i = NR_SUPER - 1; i >= 0; --i) {
		ret = try_unmount(i);
		/* -f forces reboot even if mount fails */
		if (ret && !force) return 1;
	}
	sleep(3);
	if (reboot(0x1D1E,0xC0DE,0x0123)) {
		perror("reboot");
		return 1;
	}
	return 0;
}
