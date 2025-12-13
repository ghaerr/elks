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

#define ARRAYLEN(a)     (sizeof(a)/sizeof(a[0]))

struct signames {
	char *name;
	int signo;
} signames[] = {
	{ "HUP",  SIGHUP	},
	{ "INT",  SIGINT	},
	{ "STOP", SIGSTOP	},
	{ "TSTP", SIGTSTP	},
	{ "CONT", SIGCONT	},
	{ "KILL", SIGKILL	},
	{ "TERM", SIGTERM	}
};

int atoi(const char *s)
{
    int n = 0;
    int neg = 0;

    if (*s == '-') ++s, neg = 1;
    while ((unsigned) (*s - '0') <= 9u)
        n = n * 10 + *s++ - '0';
    return neg ? 0u - n : n;
}

int signum(char *str)
{
	struct signames *s;

	for (s = signames; s < &signames[ARRAYLEN(signames)]; s++) {
		if (!strcmp(str, s->name))
			return s->signo;
	}
	return atoi(str);
}

void usage(void)
{
	errmsg("usage: kill [-<signo>|-INT|-KILL|...] pid ...\n");
	exit(1);
}

int main(int argc, char **argv)
{
	int	sig;
	int	pid;

	sig = SIGTERM;

	if (argv[1][0] == '-') {
		sig = signum(&argv[1][1]);
		if (sig <= 0)
			usage();
		argc--;
		argv++;
	} else {	
		if (argc == 1) usage();
	}
	
	while (argc-- > 1) {
		if (!(pid = atoi(*++argv)))
			usage();

		if (kill(pid, sig) < 0)
			perror(*argv);
	}
	return 0;
}
