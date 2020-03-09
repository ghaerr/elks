#include <stdio.h>
#include <unistd.h>

int
fseek(FILE *fp, long offset, int ref)
{
#if 1
   /* if __MODE_READING and no ungetc ever done can just move pointer */

   if ((fp->mode &(__MODE_READING | __MODE_UNGOT)) == __MODE_READING &&
        ( ref == SEEK_SET || ref == SEEK_CUR ))
   {
      long fpos = lseek(fp->fd, 0L, SEEK_CUR);
      if (fpos == -1) return EOF;

      if (ref == SEEK_CUR )
      {
         ref = SEEK_SET;
	 offset = fpos + offset + fp->bufpos - fp->bufread;
      }
      if (ref == SEEK_SET )
      {
         if (offset < fpos && offset >= fpos + fp->bufstart - fp->bufread )
	 {
	    fp->bufpos = offset - fpos + fp->bufread;
	    return 0;
	 }
      }
   }
#endif

   /* Use fflush to sync the pointers */

   if (fflush(fp) == EOF)
      return EOF;
   if (lseek(fp->fd, offset, ref) < 0)
      return EOF;
   return 0;
}
