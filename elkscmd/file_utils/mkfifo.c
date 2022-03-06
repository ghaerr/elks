#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "futils.h"

int main(int argc, char **argv)
{
	unsigned short newmode;
	int i, ret = 0;

	if (argc < 2) {
		errmsg("usage: mkfifo fifo_name [...]\n");
		return 1;
	}

	newmode = 0666 & ~umask(0);
	for (i = 1; i < argc; i++) {
		if (mknod(argv[i], newmode | S_IFIFO, 0) < 0) {
			errstr(argv[i]);
			errmsg(": cannot make fifo\n");
			ret = 1;
		}
	}
	return ret;
}
