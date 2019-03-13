
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <malloc.h>

DIR  *
opendir(dname)
const char *dname;
{
   struct stat st;
   int   fd;
   DIR  *p;

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

int
closedir(dirp)
DIR  *dirp;
{
   int   fd;
   fd = dirp->dd_fd;
   free(dirp->dd_buf);
   free(dirp);
   return close(fd);
}

struct dirent *
readdir(dirp)
DIR  *dirp;
{
   int cc;

   cc = _readdir(dirp->dd_fd, dirp->dd_buf, 1);
   if (cc <= 0)
      return 0;
   if (cc>1) dirp->dd_buf->d_name[cc] = 0;

   return dirp->dd_buf;
}

