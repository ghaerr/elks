/*
 * reboot.c
 *
 * Copyright 2000 Alistair Riddoch
 * ajr@ecs.soton.ac.uk
 *
 * This file may be distributed under the terms of the GNU General Public
 * License v2, or at your option any later version.
 */

/*
 * This is a small version of reboot for use in the ELKS project.
 * It is not fully functional, and may not be the most efficient
 * implementation for larger systems. It minimises memory usage and
 * code size.
 */

#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>		// FIXME can't include as sys_sleep defined which is a NOP
#include <errno.h>
#include <time.h>
#include <sys/select.h>

static unsigned int
sleep(unsigned int seconds)
{
	struct timeval timeout;

	time_t start = time(NULL);
	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;
	select(1, NULL, NULL, NULL, &timeout);
	return seconds - (time(NULL) - start);
}


int main(int argc, char **argv)
{
	sync();
	if (umount("/")) {
		perror("reboot umount");
		exit(1);
	}
	sleep(3);
	if (reboot(0x1D1E,0xC0DE,0x0123)) {
		perror("reboot");
		exit(1);
	}
}

