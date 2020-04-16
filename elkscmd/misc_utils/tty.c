#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/*
 * Type tty name
 */

int main(int argc, char **argv)
{
	register char *p;

	p = ttyname(0);
	if(argc==2 && !strcmp(argv[1], "-s"))
		;	/* silent mode,  just exit code */
	else
		printf("%s\n", (p? p: "not a tty"));
	exit(p? 0: 1);
}
