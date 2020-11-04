#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

void
rewinddir(DIR *dirp)
{
   lseek(dirp->dd_fd, 0L, SEEK_SET);
}
