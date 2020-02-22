#ifdef L_mkfifo
#include <sys/stat.h>

int
mkfifo(char * path, int mode)
{
   return mknod(path, mode | S_IFIFO, 0);
}
#endif
