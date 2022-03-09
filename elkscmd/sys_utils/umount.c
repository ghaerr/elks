/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Most simple built-in commands are here.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mount.h>

#define msg(str) write(STDOUT_FILENO, str, sizeof(str) - 1)
#define errmsg(str) write(STDERR_FILENO, str, sizeof(str) - 1)

int main(int argc, char **argv)
{
	if (argc != 2) {
		errmsg("Usage: umount <device>|<directory>\n");
		return 1;
	}
	if (umount(argv[1]) < 0) {
		perror(argv[1]);
		return 1;
	}
	if (!strcmp(argv[1], "/"))
		msg("/: Remounted read-only\n");
	return 0;
}
