/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */

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
#include "futils.h"

int main(int argc, char **argv)
{
	char	*cp;
	int	mode = 0;

	if (argc < 3)
		goto usage;

	cp = argv[1];
	while (isoctal(*cp))
		mode = mode * 8 + (*cp++ - '0');

	if (*cp) {
		errmsg("mode must be specified as an octal number (e.g. 755 is 'rwxr-xr-x').\n");
		return 1;
	}

	argc--;
	argv++;

	while (argc-- > 1) {
		if (chmod(argv[1], mode) < 0)
			perror(argv[1]);
		argv++;
	}
	return 0;

usage:
	errmsg("usage: chmod mode file [...]\n");
	return 1;
}
