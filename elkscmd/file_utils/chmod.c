/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Most simple built-in commands are here.
 */

#include "futils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
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

	mode = 0;
	cp = argv[1];
	while (isoctal(*cp))
		mode = mode * 8 + (*cp++ - '0');

	if (*cp) {
		fprintf(stderr, "Mode must be octal\n");
		exit(1);
	}
	argc--;
	argv++;

	while (argc-- > 1) {
		if (chmod(argv[1], mode) < 0)
			perror(argv[1]);
		argv++;
	}
	exit(0);
}
