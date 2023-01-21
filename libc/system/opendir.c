#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

DIR *
opendir(const char *dname)
{
    struct stat st;
    int fd;
    DIR *p;

    if (stat(dname, &st) < 0)
        return 0;

    if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        return 0;
    }
    if ((fd = open(dname, O_RDONLY)) < 0)
        return 0;

    p = malloc(sizeof(DIR));
    if (p == 0)
    {
        close(fd);
        return 0;
    }

    p->dd_buf = malloc(sizeof(struct dirent));
    if (p->dd_buf == 0)
    {
        free(p);
        close(fd);
        return 0;
    }
    p->dd_fd = fd;
    p->dd_loc = p->dd_size = 0;

    return p;
}
