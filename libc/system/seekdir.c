#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

void
seekdir(DIR *dirp, off_t pos)
{
   lseek(dirp->dd_fd, pos, SEEK_SET);
}
