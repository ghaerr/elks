#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <linuxmt/rd.h>

#define MAX_SIZE 640 /* 1 KB blocks */

int main(argc, argv)
int argc;
char **argv;
{
	int i;
	int fd;
	int size = 0;

	if ((argc != 4) && (argc != 3)) {
		fprintf(stderr, "usage: ramdisk /dev/ram? {make | kill} [size in 1 KB blocks]\n");
		exit(1);
	}

	if (argc == 4)
		sscanf(argv[3], "%d", &size);
	else
		size = 64; /* default */
	
	if ((size < 1) || (size > MAX_SIZE)) {
		fprintf(stderr, "ramdisk: invalid size; use integer in range of 1 .. %d, ramdisk will round it up to nearest multiple of 4 KB\n", MAX_SIZE);
		exit(1);
	}
	if (( fd = open(argv[1], 0) ) == -1) {
		perror("ramdisk");
		exit(1);
	}
	if (strcmp(argv[2],"make") == 0) {
		/* recalculate size to # of 4 KB pages */
			if ((size % 4) != 0) {
				fprintf(stdout, "ramdisk: rounding size up to %d KB ...\n", ((size / 4) + 1) * 4);
				size = size / 4 + 1;
			} else {
				size = size / 4;
			}

		if (ioctl(fd, RDCREATE, size)) {
			perror("ramdisk");
			exit(1);
		}
		fprintf(stdout,"ramdisk: %d KB ramdisk created on %s\n", size * 4, argv[1]);
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
