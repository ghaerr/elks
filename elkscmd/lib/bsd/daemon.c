/*
 * BSD libc compatability functions.
 *
 * Written from scratch by:-
 *
 * Al Riddoch <ajr@ecs.soton.ac.uk>
 *
 * Copyright (C) 1999 Al Riddoch
 *
 */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "include/bsd_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int daemon(int nochdir, int noclose)
{
	int fd;
	int i = fork();

	if (i < 0) {
		return -1;
	}

	if (i > 0) {
		exit(0);
	}

	if (setsid() < 0) {
		return -1;
	}

	if (!nochdir) {
		chdir("/");
	}

	if (!noclose) {
		if ((fd = open("/dev/null", O_RDWR)) == -1) {
			return 0;
		}
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		if (fd > STDERR_FILENO) {
			close(fd);
		}
	}
	return 0;
}
