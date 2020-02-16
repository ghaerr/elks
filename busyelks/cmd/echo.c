/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Most simple built-in commands are here.
 */

#include "../sash.h"

#include <unistd.h>

#include "cmd.h"

int
echo_main(int argc, char * argv[])
{
	int first;
	char *sptr;

	first = 0;
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
