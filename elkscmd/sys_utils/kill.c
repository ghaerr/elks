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

static void usage()
{
	write(STDOUT_FILENO, "Send signal to process\n\nusage: kill [signal] PID\n\n",50);
	write(STDOUT_FILENO, "  default  send TERM signal\n",28);
	write(STDOUT_FILENO, "  -HUP     tell a process to shutdown and restart\n",50);
	write(STDOUT_FILENO, "  -INT     send ctrl+c signal to process\n",41);
	write(STDOUT_FILENO, "  -QUIT    stop process and tell it to create a core dump file\n",63);
	write(STDOUT_FILENO, "  -KILL    stop process\n",24);
	write(STDOUT_FILENO, "  -H       print this message\n",30);
	write(STDOUT_FILENO, "\n  e.g. kill -KILL 5\n\n",22);
}

void main ( int argc, char **argv ) {  
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
		else if ((strcmp(cp, "H") == 0) || (strcmp(cp, "h") == 0))
			usage();
		else {
			sig = 0;
			while (isdecimal(*cp))
				sig = sig * 10 + *cp++ - '0';

			if (*cp) {
				write(STDERR_FILENO, "Unknown signal\n", 15);
				exit(1);
			}
		}
		argc--;
		argv++;
	} else {	
		if (argc == 1) usage();
	}
	
	while (argc-- > 1) {
		cp = *++argv;
		pid = 0;
		while (isdecimal(*cp))
			pid = pid * 10 + *cp++ - '0';

		if (*cp) {
			usage();	
			write(STDERR_FILENO, "\n  Error: non-numeric pid!\n\n", 27);			
			exit(1);
		}

		if (kill(pid, sig) < 0)
			perror(*argv);
	}
	exit(0);
}
