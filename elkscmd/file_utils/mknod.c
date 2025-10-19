#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <linuxmt/kdev_t.h>
#include "futils.h"

int main(int argc, char **argv)
{
	unsigned short newmode;
	unsigned short filetype;
	int major,minor;
	
	newmode = 0666 & ~umask(0);
	
	if (argv[1] && argv[1][0] == '-' && argv[1][1] == 'p') {
		/* preserve option */
		struct stat sb;
		if (stat(argv[2], &sb) == 0) {
			if (S_ISCHR(sb.st_mode) && argv[3] && (argv[3][0] == 'c' || argv[3][0] == 'u')) return 0;
			if (S_ISBLK(sb.st_mode)  && argv[3] && argv[3][0] == 'b') return 0;
			if (S_ISFIFO(sb.st_mode) && argv[3] && argv[3][0] == 'p') return 0;
		}
		/* valid node not there yet, so we advance and create it */
		argc--;
		argv++;
	}

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
		
		if (major > 0 && minor >= 0) {
			if (mknod (argv[1], newmode | filetype, MKDEV(major, minor))) {
				errstr(argv[1]);
				errmsg(": cannot make device\n");
				return 1;
			}
		} else goto usage;
	} else if ((argc == 3) && (argv[2][0] == 'p')) {
		if (mknod (argv[1],newmode | S_IFIFO, 0)) {
			errstr(argv[1]);
			errmsg(": cannot make fifo\n");
			return 1;
		}

	} else goto usage;
	return 0;

usage:
	errmsg("usage: mknod [-p] device [bcup] major minor\n");
	return 1;
}
