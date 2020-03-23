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
	char		*cp;
	int		uid;
	struct passwd	*pwd;
	struct stat	statbuf;

	if (argc < 3) goto usage;

	cp = argv[1];
	if (isdecimal(*cp)) {
		uid = 0;
		while (isdecimal(*cp))
			uid = uid * 10 + (*cp++ - '0');

		if (*cp) {
			fprintf(stderr, "Bad uid value\n");
			goto usage;
		}
	} else {
		pwd = getpwnam(cp);
		if (pwd == NULL) {
			fprintf(stderr, "Unknown user name\n");
			goto usage;
		}

		uid = pwd->pw_uid;
	}

	argc--;
	argv++;

	while (argc-- > 1) {
		argv++;
		if ((stat(*argv, &statbuf) < 0) ||
			(chown(*argv, uid, statbuf.st_gid) < 0))
				perror(*argv);
	}
	return 0;

usage:
	fprintf(stderr, "usage: %s new_owner file1 [file2] ...\n", argv[0]);
	return 1;
}
