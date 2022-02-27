#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/*
 * Display tty name
 */

#define msgout(str) write(STDOUT_FILENO, str, sizeof(str) - 1)
#define strout(str) write(STDOUT_FILENO, str, strlen(str))

int main(int argc, char **argv)
{
	char *p = ttyname(0);

	if(argc == 2 && !strcmp(argv[1], "-s"))
		;	/* silent mode,  just exit code */
	else {
		if (p) strout(p);
		else msgout("not a tty");
		msgout("\n");
	}
	exit(p? 0: 1);
}
