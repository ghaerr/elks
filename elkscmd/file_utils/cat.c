/*
 * /bin/cat for ELKS; mark II
 *
 * 1997 MTK, other insignificant people
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static char readbuf[BUFSIZ];    /* use disk block size for stack limit and efficiency*/

static int copyfd(int fd)
{
	int n;

	while ((n = read(fd, readbuf, sizeof(readbuf))) > 0) {
		if (write(STDOUT_FILENO, readbuf, n) != n)
			return 1;
    }
	return n < 0? 1: 0;
}

int main(int argc, char **argv)
{
	int i, fd;

	if (argc <= 1) {
		if (copyfd(STDIN_FILENO)) {
			perror("stdin");
			return 1;
		}
	} else {
		for (i = 1; i < argc; i++) {
			errno = 0;
			if (argv[i][0] == '-' && argv[i][1] == '\0')
				fd = STDIN_FILENO;
			else if ((fd = open(argv[i], O_RDONLY)) < 0) {
				perror(argv[i]);
				return 1;
			}
			if (copyfd(fd)) {
				perror(argv[i]);
				close(fd);
				return 1;
			}
			if (fd != STDIN_FILENO)
				close(fd);
		}
	}
	return 0;
}
