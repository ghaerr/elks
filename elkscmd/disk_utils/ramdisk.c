#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linuxmt/rd.h>

int main(argc, argv)
int argc;
char **argv;
{
	int i,fd;

	if (argc != 3) {
		write(STDERR_FILENO,"usage: ramdisk /dev/ram? {make | kill}\n", 39);
		exit(1);
	}
	if (( fd = open(argv[1], 0) ) == -1) {
		perror("ramdisk");
		exit(1);
	}
	if (strcmp(argv[2],"make") == 0) {
		if (ioctl(fd, RDCREATE, 0)) {
			perror("ramdisk");
			exit(1);
		}
		write(STDOUT_FILENO,"64K ramdisk created on ",23);
		write(STDOUT_FILENO, argv[1], strlen(argv[1]));
		write(STDOUT_FILENO, "\n", 1);
		exit(0);
	}
	if (strcmp(argv[2],"kill") == 0) {
		if (ioctl(fd, RDDESTROY, 0)) {
			perror("ramdisk");
			exit(1);
		}
		write(STDOUT_FILENO,"64K ramdisk destroyed on ",25);
		write(STDOUT_FILENO, argv[1], strlen(argv[1]));
		write(STDOUT_FILENO, "\n", 1);
		exit(0);
	}
	write(STDERR_FILENO,"usage: ramdisk filename {make | kill}\n", 38);
}
