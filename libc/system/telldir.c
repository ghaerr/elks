#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

off_t
telldir(DIR *dirp)
{
   return lseek(dirp->dd_fd, 0L, SEEK_CUR);
}
