#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linuxmt/rd.h>

#define MAX_SIZE 640 /* 1 KB blocks */

int main(argc, argv)
int argc;
char **argv;
{
	int fd;
	int size = 0;

	if ((argc != 4) && (argc != 3)) {
		fprintf(stderr, "usage: ramdisk /dev/rd? {make | kill} [size in 1 KB blocks]\n");
		exit(1);
	}

	if (argc == 4)
		sscanf(argv[3], "%d", &size);
	else
		size = 64; /* default */
	
	if ((size < 1) || (size > MAX_SIZE)) {
		fprintf(stderr, "ramdisk: invalid size; use integer in range of 1 .. %d\n", MAX_SIZE);
		exit(1);
	}
	if (( fd = open(argv[1], 0) ) == -1) {
		perror("ramdisk");
		exit(1);
	}
	if (strcmp(argv[2],"make") == 0) {
		if (ioctl(fd, RDCREATE, size)) {
			perror("ramdisk");
			exit(1);
		}
		fprintf(stdout,"ramdisk: %d KB ramdisk created on %s\n", size, argv[1]);
		exit(0);
	}
	if (strcmp(argv[2],"kill") == 0) {
		if (ioctl(fd, RDDESTROY, 0)) {
			perror("ramdisk");
			exit(1);
		}
		fprintf(stdout,"ramdisk destroyed on %s\n", argv[1]);
		exit(0);
	}
}
