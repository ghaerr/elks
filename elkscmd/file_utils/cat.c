/*
 * /bin/cat for ELKS; mark II
 *
 * 1997 MTK, other insignificant people
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define CAT_BUF_SIZE	BUFSIZ		/* use disk block size for stack limit and efficiency*/

static char readbuf[CAT_BUF_SIZE];

static int dumpfile(int fd)
{
	int nred;

	while ((nred = read(fd, readbuf, CAT_BUF_SIZE)) > 0) {
		write(STDOUT_FILENO, readbuf, nred);
	}
	if (nred < 0) return -1;
	return 0;
}

int main(int argc, char **argv)
{
	int i, fd = -1;

	if (argc <= 1) {
		if (dumpfile(STDIN_FILENO)) {
			perror("stdin");
			return 1;
		}
	} else {
		for (i = 1; i < argc; i++) {
			errno = 0;
			if ((fd = open(argv[i], O_RDONLY)) < 0) {
				perror(argv[i]);
				return 1;
			} else {
				if (dumpfile(fd)) {
					perror(argv[i]);
					close(fd);
					return 1;
				}
				close(fd);
			}
		}
	}
	return 0;
}
