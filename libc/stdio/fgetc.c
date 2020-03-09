#include <stdio.h>

int fgetc(FILE *fp)
{
   size_t ch;

   if (fp->mode & __MODE_WRITING)
      fflush(fp);

#if __MODE_IOTRAN && !O_BINARY
 try_again:
#endif
   /* Can't read or there's been an EOF or error then return EOF */
   if ((fp->mode & (__MODE_READ | __MODE_EOF | __MODE_ERR)) != __MODE_READ)
      return EOF;

   /* Nothing in the buffer - fill it up */
   if (fp->bufpos >= fp->bufread)
   {
      /* Bind stdin to stdout if it's open and line buffered */
      if( fp == stdin && stdout->fd >= 0 && (stdout->mode & _IOLBF ))
         fflush(stdout);

      fp->bufpos = fp->bufread = fp->bufstart;
      ch = fread(fp->bufpos, 1, fp->bufend - fp->bufstart, fp);
      if (ch == 0)
	 return EOF;
      fp->bufread += ch;
      fp->mode |= __MODE_READING;
      fp->mode &= ~__MODE_UNGOT;
   }
   ch = *(fp->bufpos++);

#if __MODE_IOTRAN && !O_BINARY
   /* In MSDOS translation mode; WARN: Doesn't work with UNIX macro */
   if (ch == '\r' && (fp->mode & __MODE_IOTRAN))
      goto try_again;
#endif

   return ch;
}
