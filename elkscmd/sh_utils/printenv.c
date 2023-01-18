/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */

#include "shutils.h"
#include <unistd.h>

int main(int argc, char **argv)
{
	char		**env = environ;
	char		*eptr;
	int		len;

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
