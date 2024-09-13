#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/linksym.h>

int
fflush(FILE *fp)
{
   int   len, cc, rv=0;
   unsigned char * bstart;

   __LINK_SYMBOL(__stdio_init);
   if (fp == NULL)		/* On NULL flush the lot. */
   {
      if (fflush(stdin))
	 return EOF;
      if (fflush(stdout))
	 return EOF;
      if (fflush(stderr))
	 return EOF;

      for (fp = __IO_list; fp; fp = fp->next)
	 if (fflush(fp))
	    return EOF;

      return 0;
   }

   /*
    * Immediately error out if this is a fake FILE from snprintf etc.  Do
    * _not_ clear the buffer in this case!
    */
   if (fp->fd < 0)
   {
      fp->mode |= __MODE_ERR;
      return EOF;
   }

   /* If there's output data pending */
   if (fp->mode & __MODE_WRITING)
   {
      len = fp->bufpos - fp->bufstart;

      if (len)
      {
	 bstart = fp->bufstart;
	 /*
	  * The loop is so we don't get upset by signals or partial writes.
	  */
	 do
	 {
	    cc = write(fp->fd, bstart, len);
	    if( cc > 0 )
	    {
	       bstart+=cc; len-=cc;
	    }
	 }
	 while ( len>0 && (cc>0 || (cc == -1 && errno == EINTR)));
	 /*
	  * If we get here with len!=0 there was an error, exactly what to
	  * do about it is another matter ...
	  *
	  * I'll just clear the buffer.
	  */
	 if (len)
	 {
	    fp->mode |= __MODE_ERR;
	    rv = EOF;
	 }
      }
   }
   /* If there's data in the buffer sychronise the file positions */
   else if (fp->mode & __MODE_READING)
   {
      /* Humm, I think this means sync the file like fpurge() ... */
      /* Anyway the user isn't supposed to call this function when reading */

      len = fp->bufread - fp->bufpos;	/* Bytes buffered but unread */
      /* If it's a file, make it good */
      if (len > 0 && lseek(fp->fd, (long)-len, 1) < 0)
      {
	 /* Hummm - Not certain here, I don't think this is reported */
	 /*
	  * fp->mode |= __MODE_ERR; return EOF;
	  */
      }
   }

   /* All done, no problem */
   fp->mode &= (~(__MODE_READING|__MODE_WRITING|__MODE_EOF|__MODE_UNGOT));
   fp->bufread = fp->bufwrite = fp->bufpos = fp->bufstart;
   return rv;
}
