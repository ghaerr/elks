/*
 * Set console device
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

int main(int argc, char **argv)
{
	int fd;
	struct stat statbuf;

	if (argc != 2) {
		printf("Usage: console </dev/ttyXX>\n");
		return 1;
	}

	if ((fd = open(argv[1], O_RDWR)) < 0) {
		perror(argv[1]);
		return 1;
	}
	if (fstat(fd, &statbuf) < 0) {
		perror("fstat");
		return 1;
	}
	if (ioctl(fd, TIOSETCONSOLE, &statbuf.st_rdev) != 0) {
		perror(argv[1]);
		return 1;
	}

	close(fd);
	return 0;
}
