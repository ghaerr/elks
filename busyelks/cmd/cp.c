/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Most simple built-in commands are here.
 */

#include "../sash.h"

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

#include "cmd.h"

int
cp_main(int argc, char * argv[])
{
	int	dirflag;
	char	*srcname;
	char	*destname;
	char	*lastarg;

	if (argc < 3) goto usage;

	lastarg = argv[argc - 1];

	dirflag = isadir(lastarg);

	if ((argc > 3) && !dirflag) {
		fprintf(stderr, "%s: not a directory\n", lastarg);
		exit(1);
	}

	while (argc-- > 2) {
		srcname = argv[1];
		destname = lastarg;
		if (dirflag) destname = buildname(destname, srcname);

		if (!copyfile(*++argv, destname, 0)) goto error_copy;
	}
	exit(0);

error_copy:
	fprintf(stderr, "Failed to copy %s -> %s\n", srcname, destname);
	exit(1);
usage:
	fprintf(stderr, "usage: %s source_file dest_file\n", argv[0]);
	fprintf(stderr, "       %s file1 file2 ... dest_directory\n", argv[0]);
	exit(1);
}
