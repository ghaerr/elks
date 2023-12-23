#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "futils.h"

static int remove_dir(char *name, int f)
{
    int ret, once = 1;
    char *line;

    while (((ret = rmdir(name)) == 0) && ((line = strrchr(name,'/')) != NULL) && f) {
        while ((line > name) && (*line == '/'))
            --line;
        line[1] = 0;
        once = 0;
    }
    return (ret && once);
}

int main(int argc, char **argv)
{
    int i, parent = 0, force = 0, ret = 0;

    if (argc < 2) goto usage;

    while (argv[1][0] == '-') {
        switch (argv[1][1]) {
        case 'p': parent = 1; break;
        case 'f': force = 1; break;
        default: goto usage;
        }
        argv++;
        argc--;
    }

    for (i = 1; i < argc; i++) {
            while (argv[i][strlen(argv[i])-1] == '/')
                argv[i][strlen(argv[i])-1] = '\0';
            if (remove_dir(argv[i],parent)) {
                errstr(argv[i]);
                errmsg(": cannot remove directory\n");
                if (!force)
                    ret = 1;
            }
    }
    return ret;

usage:
    errmsg("usage: rmdir [-pf] directory [...]\n");
    return 1;
}
