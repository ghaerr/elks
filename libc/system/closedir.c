#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

int
closedir(DIR *dirp)
{
   int   fd;
   fd = dirp->dd_fd;
   free(dirp->dd_buf);
   free(dirp);
   return close(fd);
}
