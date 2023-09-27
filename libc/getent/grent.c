/*
 * Sep 2023 Greg Haerr
 */
#include <unistd.h>
#include <stdlib.h>
#include <grp.h>
#include <fcntl.h>
#include <paths.h>

static int __grfd = -1;     /* file descriptor for group file */

void
setgrent(void)
{
    if (__grfd < 0)
        __grfd = open(_PATH_GROUP, O_RDONLY);
    lseek(__grfd, 0L, SEEK_SET);
}

void
endgrent(void)
{
    if (__grfd >= 0) {
        close(__grfd);
        __grfd = -1;
    }
}

struct group *
getgrent(void)
{
    if (__grfd < 0) {
        setgrent();
        if (__grfd < 0)
            return NULL;
    }
    return __getgrent(__grfd);
}
