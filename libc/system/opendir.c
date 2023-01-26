#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

DIR *
opendir(const char *dname)
{
    int fd;
    DIR *p;
    struct stat st;

    if ((fd = open(dname, O_RDONLY)) < 0)
        return NULL;

    if (fstat(fd, &st) < 0 || !S_ISDIR(st.st_mode)) {
        close(fd);
        errno = ENOTDIR;
        return NULL;
    }

    p = malloc(sizeof(DIR) + sizeof(struct dirent));
    if (p == NULL) {
        close(fd);
        errno = ENOMEM;
        return NULL;
    }

    p->dd_buf = (struct dirent *)((char *)p + sizeof(DIR));
    p->dd_fd = fd;
    p->dd_loc = p->dd_size = 0;

    return p;
}
