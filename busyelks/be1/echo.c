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
echo_main(argc, argv)
	char	**argv;
{
	BOOL	first;
	char *	sptr;

	first = TRUE;
	while (argc-- > 1) {
		if (!first)
			write(STDOUT_FILENO, " ", 1);
		first = FALSE;
		sptr = *++argv;
		write(STDOUT_FILENO, sptr, strlen(sptr));
	}
	write(STDOUT_FILENO, "\n", 1);
	exit(0);
}
