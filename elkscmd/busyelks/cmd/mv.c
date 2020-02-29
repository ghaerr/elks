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
mv_main(argc, argv)
	char	**argv;
{
	int	dirflag;
	char	*srcname;
	char	*destname;
	char	*lastarg;

	lastarg = argv[argc - 1];

	dirflag = isadir(lastarg);

	if ((argc > 3) && !dirflag) {
		write(STDERR_FILENO, lastarg, strlen(lastarg));
		write(STDERR_FILENO, ": not a directory\n", 18);
		exit(1);
	}

	while (argc-- > 2) {
		srcname = *(++argv);
		if (access(srcname, 0) < 0) {
			perror(srcname);
			continue;
		}

		destname = lastarg;
		if (dirflag)
			destname = buildname(destname, srcname);

		if (rename(srcname, destname) >= 0)
			continue;

		if (errno != EXDEV) {
			perror(destname);
			continue;
		}

		if (!copyfile(srcname, destname, TRUE))
			continue;

		if (unlink(srcname) < 0)
			perror(srcname);
	}
	exit(0);
}

