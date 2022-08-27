#include <dirent.h>

struct dirent *
readdir(DIR *dirp)
{
   int cc;

   cc = _readdir(dirp->dd_fd, dirp->dd_buf, 1);
   if (cc <= 0)
      return 0;
   return dirp->dd_buf;
}
