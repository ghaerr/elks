#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include "futils.h"

#ifndef makedev
#define makedev(maj, min)  (((maj) << 8) | (min))
#endif

int main(int argc, char **argv)
{
	unsigned short newmode;
	unsigned short filetype;
	int major,minor;
	
	newmode = 0666 & ~umask(0);
	
	if (argc == 5) {
		switch(argv[2][0]) {
		case 'b':
			filetype = S_IFBLK;
			break;
		case 'c':
		case 'u':
			filetype = S_IFCHR;
			break;
		default:
			goto usage;
		}
		major = atoi(argv[3]);
		minor = atoi(argv[4]);
		
		if (errno != ERANGE)
			if (mknod (argv[1], newmode | filetype, makedev(major, minor))) {
				errstr(argv[1]);
				errmsg(": cannot make device\n");
				return 1;
			}
	} else if ((argc == 3) && (argv[2][0] == 'p')) {
		if (mknod (argv[1],newmode | S_IFIFO, 0)) {
			errstr(argv[1]);
			errmsg(": cannot make fifo\n");
			return 1;
		}

	} else goto usage;
	return 0;

usage:
	errmsg("usage: mknod [bcup] device major minor\n");
	return 1;
}
