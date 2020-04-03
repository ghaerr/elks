/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Most simple built-in commands are here.
 */

#include "shutils.h"

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
	char		**env;
	char		*eptr;
	extern char	**environ;
	int		len;

	env = environ;

	if (argc == 1) {
		while (*env) {
			eptr = *env++;
			write(STDOUT_FILENO, eptr, strlen(eptr));
			write(STDOUT_FILENO, "\n", 1);
		}
		return 0;
	}

	len = strlen(argv[1]);
	while (*env) {
		if ((strlen(*env) > len) && (env[0][len] == '=') &&
			(memcmp(argv[1], *env, len) == 0))
		{
			eptr = &env[0][len+1];
			write(STDOUT_FILENO, eptr, strlen(eptr));
			write(STDOUT_FILENO, "\n", 1);
			return 0;
		}
		env++;
	}
	return 0;
}
