/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Most simple built-in commands are here.
 */

#include "../sash.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <utime.h>
#include <errno.h>

void
cp_main(argc, argv)
	char	**argv;
{
	BOOL	dirflag;
	char	*srcname;
	char	*destname;
	char	*lastarg;

	lastarg = argv[argc - 1];

	dirflag = isadir(lastarg);

	if ((argc > 3) && !dirflag) {
		fprintf(stderr, "%s: not a directory\n", lastarg);
		exit(1);
	}

	while (argc-- > 2) {
		srcname = argv[1];
		destname = lastarg;
		if (dirflag)
			destname = buildname(destname, srcname);

		(void) copyfile(*++argv, destname, FALSE);
	}
	exit(0);
}


