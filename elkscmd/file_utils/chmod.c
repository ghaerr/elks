/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Most simple built-in commands are here.
 */

#include "futils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <utime.h>
#include <errno.h>

int main(int argc, char **argv)
{
	char	*cp;
	int	mode;

	if (argc < 3) {
		fprintf(stderr, "You must specify a mode number and at least one file or directory.\n");
		goto usage;
	}

	mode = 0;
	cp = argv[1];
	while (isoctal(*cp))
		mode = mode * 8 + (*cp++ - '0');

	if (*cp) {
		fprintf(stderr, "Mode must be an octal number\n");
		goto usage;
	}
	argc--;
	argv++;

	while (argc-- > 1) {
		if (chmod(argv[1], mode) < 0)
			perror(argv[1]);
		argv++;
	}
	exit(0);

usage:
	fprintf(stderr, "usage: %s mode file1 [file2] ...\n", argv[0]);
	fprintf(stderr, "Mode must be specified as an octal number (i.e. 755 is 'rwxr-xr-x')\n");
	exit(1);
}
