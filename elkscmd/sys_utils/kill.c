/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define errmsg(str)     write(STDERR_FILENO, str, sizeof(str) - 1)

static void usage()
{
	errmsg("usage: kill [-<signo>|-INT|-KILL|-HUP] PID...\n");
	exit(1);
}

int main(int argc, char **argv)
{
	char	*cp;
	int	sig;
	int	pid;

	sig = SIGTERM;

	if (argv[1][0] == '-') {
		cp = &argv[1][1];
		if (strcmp(cp, "HUP") == 0)
			sig = SIGHUP;
		else if (strcmp(cp, "INT") == 0)
			sig = SIGINT;
		else if (strcmp(cp, "KILL") == 0)
			sig = SIGKILL;
		else {
			if (!(sig = atoi(cp)))
				usage();
		}
		argc--;
		argv++;
	} else {	
		if (argc == 1) usage();
	}
	
	while (argc-- > 1) {
		cp = *++argv;
		if (!(pid = atoi(cp)))
			usage();

		if (kill(pid, sig) < 0)
			perror(*argv);
	}
	return 0;
}
