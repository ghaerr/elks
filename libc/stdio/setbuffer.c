#include <stdio.h>
#include <stdlib.h>

void
setbuffer(FILE *fp, char *buf, int size)
{
   fflush(fp);
   if( fp->mode & __MODE_FREEBUF ) free(fp->bufstart);
   fp->mode &= ~(__MODE_FREEBUF|__MODE_BUF);

   if( buf == 0 )
   {
      fp->bufstart = fp->unbuf;
      fp->bufend = fp->unbuf + sizeof(fp->unbuf);
      fp->mode |= _IONBF;
   }
   else
   {
      fp->bufstart = (unsigned char *)buf;
      fp->bufend = (unsigned char *)buf+size;
      fp->mode |= _IOFBF;
   }
   fp->bufpos = fp->bufread = fp->bufwrite = fp->bufstart;
}
