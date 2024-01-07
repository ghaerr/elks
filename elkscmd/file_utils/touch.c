#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include "futils.h"

int
main(int argc, char **argv)
{
    int ncreate = 0;
    int err = 0;
    struct stat sbuf;

    if (argc < 2) {
        errmsg("usage: touch file [...]\n");
        return 1;
    }
    if ((argv[1][0] == '-') && (argv[1][1] == 'c')) {
        ncreate = 1;
        argv++;
    }

    for (argv++; *argv; argv++) {
        if (**argv != '-') {
            if (stat(*argv, &sbuf)) {
                if (!ncreate) {
                    int fd = creat(*argv, 0666);
                    if (fd < 0) {
                        errstr(*argv);
                        errmsg(": cannot create file\n");
                        err = 1;
                    } else
                        close(fd);
                }
            } else
                err |= utime(*argv, NULL);
        }
    }
    return (err ? 1 : 0);
}
