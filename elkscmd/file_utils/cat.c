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

#define CAT_BUF_SIZE 4096

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
	int i, fd;

	if (argc <= 1) {
		if (dumpfile(STDIN_FILENO)) goto error_read;
	} else {
		for (i = 1; i < argc; i++) {
			errno = 0;
			fd = open(argv[i], O_RDONLY);
			if (fd == -1) {
				goto error_read;
			} else {
				if (dumpfile(fd)) goto error_read;
				close(fd);
			}
		}
	}
	exit(0);

error_read:
	fprintf(stderr, "%s: %s: %s\n",
		argv[0], argv[i], strerror(errno));
	close(fd);
	exit(1);
}
