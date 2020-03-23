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
#include <fcntl.h>
#include <sys/mount.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		write(STDERR_FILENO, "Usage: umount <device>|<directory>\n", 35);
		return 1;
	}
	if (umount(argv[1]) < 0) {
		perror(argv[1]);
		exit(1);
	}
	return 0;
}
