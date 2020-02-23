#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * Like fread, fwrite will often be used to write out large chunks of
 * data; calling write() directly can be a big win in this case.
 * 
 * But first we check to see if there's space in the buffer.
 * 
 * Again this ignores __MODE__IOTRAN.
 */
int
fwrite(char *buf, int size, int nelm, FILE *fp)
{
   register int v;
   int   len;
   unsigned bytes, put;

   /* If last op was a read ... note fflush may change fp->mode and ret OK */
   if ((fp->mode & __MODE_READING) && fflush(fp))
      return 0;

   v = fp->mode;
   /* Can't write or there's been an EOF or error then return 0 */
   if ((v & (__MODE_WRITE | __MODE_EOF | __MODE_ERR)) != __MODE_WRITE)
      return 0;

   /* This could be long, doesn't seem much point tho */
   bytes = size * nelm;

   len = fp->bufend - fp->bufpos;

   /* Flush the buffer if not enough room */
   if (bytes > len)
      if (fflush(fp))
	 return 0;

   len = fp->bufend - fp->bufpos;
   if (bytes <= len)		/* It'll fit in the buffer ? */
   {
      register int do_flush=0;
      fp->mode |= __MODE_WRITING;
      memcpy(fp->bufpos, buf, bytes);
      if (v & _IOLBF)
      {
         if(memchr(fp->bufpos, '\n', bytes))
	    do_flush=1;
      }
      fp->bufpos += bytes;

      /* If we're unbuffered or line buffered and have seen nl */
      if (do_flush || (v & _IONBF) != 0)
	 fflush(fp);

      return nelm;
   }
   else
      /* Too big for the buffer */
   {
      put = bytes;
      do
      {
         len = write(fp->fd, buf, bytes);
	 if( len > 0 )
	 {
	    buf+=len; bytes-=len;
	 }
      }
      while (len > 0 || (len == -1 && errno == EINTR));

      if (len < 0)
	 fp->mode |= __MODE_ERR;

      put -= bytes;
   }

   return put / size;
}
