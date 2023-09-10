#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <paths.h>

char *ttyname(int fd)
{
    DIR  *dp;
    struct dirent *d;
    struct stat src, dst;
    static char path[MAXNAMLEN+6] = _PATH_DEVSL;     /* /dev/ */
#define NAMEOFF     (sizeof(_PATH_DEVSL) - 1)

    if (fstat(fd, &src) < 0)
        return NULL;
    if (!isatty(fd)) {
        errno = ENOTTY;
        return NULL;
    }

    dp = opendir(_PATH_DEV);
    if (!dp)
        return NULL;

    while ((d = readdir(dp)) != 0) {
        if (d->d_name[0] == '.')
            continue;
        strcpy(&path[NAMEOFF], d->d_name);
        if (stat(path, &dst) == 0) {
            if (src.st_dev == dst.st_dev && src.st_ino == dst.st_ino) {
                closedir(dp);
                return path;
            }
        }
    }
    closedir(dp);
    return NULL;
}
