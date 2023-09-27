#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linuxmt/rd.h>

#define MAX_SIZE 640 /* 1 KB blocks */

#define errmsg(str) write(STDERR_FILENO, str, sizeof(str) - 1)
#define errstr(str) write(STDERR_FILENO, str, strlen(str))

static int usage(void)
{
    errmsg("usage: ramdisk /dev/{rd0|ssd} {make | kill} [size in 1Kb blocks]\n");
    return 1;
}

int main(int argc, char **argv)
{
    int fd;
    int size = 0;

    if ((argc != 4) && (argc != 3)) {
        return usage();
    }

    if (argc == 4)
        size = atoi(argv[3]);
    else
        size = 64; /* default */

    if (size < 1 || size > MAX_SIZE) {
        errmsg("ramdisk: invalid size, range is 1-640\n");
        return 1;
    }
    if (( fd = open(argv[1], 0) ) == -1) {
        perror("ramdisk");
        return 1;
    }
    if (strcmp(argv[2],"make") == 0) {
        if (ioctl(fd, RDCREATE, size)) {
            perror("ramdisk");
            return 1;
        }
        errmsg("ramdisk: ");
        errstr(itoa(size));
        errmsg("Kb ramdisk created on ");
        errstr(argv[1]);
        errmsg("\n");
        return 0;
    } else if (strcmp(argv[2],"kill") == 0) {
        if (ioctl(fd, RDDESTROY, 0)) {
            if (errno == ENXIO) /* ramdisk present but not inited */
                return 0;
            perror("ramdisk");
            return 1;
        }
        errmsg("ramdisk destroyed on ");
        errstr(argv[1]);
        errmsg("\n");
        return 0;
    } else return usage();
    return 0;
}
