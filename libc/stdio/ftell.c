#ifdef L_ftell
#include <stdio.h>

long
ftell(FILE *fp)
{
   long rv;
   if (fflush(fp) == EOF)
      return EOF;
   return lseek(fp->fd, 0L, SEEK_CUR);
}
#endif
