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
	printf("Send signal to process\n\nusage: kill [signal] PID\n\n");
	printf("  default  send TERM signal\n");
	printf("  -HUP     tell a process to shutdown and restart\n");
	printf("  -INT     send ctrl+c signal to process\n");
	printf("  -QUIT    stop process and tell it to create a core dump file\n");
	printf("  -KILL    stop process\n");
	printf("  -H       print this message\n");
	printf("\n  e.g. kill KILL 5\n");
}

void
main(argc, argv)
	char	**argv;
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
		else if ((strcmp(cp, "H") == 0) || (strcmp(cp, "h") == 0))
			usage();
		else {
			sig = 0;
			while (isdecimal(*cp))
				sig = sig * 10 + *cp++ - '0';

			if (*cp) {
				printf("Unknown signal\n", 15);
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
			printf("\n  Error: non-numeric pid!\n\n", 16);			
			exit(1);
		}

		if (kill(pid, sig) < 0)
			perror(*argv);
	}
	exit(0);
}
