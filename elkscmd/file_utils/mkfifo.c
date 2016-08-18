#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>


int main(int argc, char **argv)
{
	unsigned short newmode;
	int i, er=0;

	if (argc < 2) goto usage;
	newmode = 0666 & ~umask(0);
	for (i = 1; i < argc; i++) {
/* The first line below mith mkfifo is used in the GNU version but there
   is no mkfifo call in elks libc yet */
/*		if (mkfifo (argv[i],newmode)) */
		if (mknod(argv[i], newmode | S_IFIFO, 0))
		{
			fprintf(stderr, "mkfifo: cannot make fifo %s",argv[i]);
			er &= 1;
		}
	}
	exit(er);

usage:
	fprintf(stderr, "usage: %s fifo_name [fifo_name] ...\n", argv[0]);
	exit(1);
}
