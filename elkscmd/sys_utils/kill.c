/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Most simple built-in commands are here.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <utime.h>
#include <errno.h>

#define isdecimal(ch)	(((ch) >= '0') && ((ch) <= '9'))

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
		else if (strcmp(cp, "QUIT") == 0)
			sig = SIGQUIT;
		else if (strcmp(cp, "KILL") == 0)
			sig = SIGKILL;
		else {
			sig = 0;
			while (isdecimal(*cp))
				sig = sig * 10 + *cp++ - '0';

			if (*cp) {
				write(stderr, "Unknown signal\n", 15);
				exit(1);
			}
		}
		argc--;
		argv++;
	}

	while (argc-- > 1) {
		cp = *++argv;
		pid = 0;
		while (isdecimal(*cp))
			pid = pid * 10 + *cp++ - '0';

		if (*cp) {
			write(STDERR_FILENO, "Non-numeric pid\n", 16);
			exit(1);
		}

		if (kill(pid, sig) < 0)
			perror(*argv);
	}
	exit(0);
}
