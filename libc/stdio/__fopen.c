#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

FILE *
__fopen(const char * fname, int fd, FILE * fp, const char * mode)
{
   int   open_mode = 0;
#if __MODE_IOTRAN && !O_BINARY
   int	 do_iosense = 1;
#endif
   int   fopen_mode = 0;
   FILE *nfp = 0;

   /* If we've got an fp close the old one (freopen) */
   if (fp)
   {
      /* Careful, don't de-allocate it */
      fopen_mode |= (fp->mode & (__MODE_BUF | __MODE_FREEFIL | __MODE_FREEBUF));
      fp->mode &= ~(__MODE_FREEFIL | __MODE_FREEBUF);
      fclose(fp);
   }

   /* decode the new open mode */
   while (*mode)
      switch (*mode++)
      {
      case 'r':
	 fopen_mode |= __MODE_READ;
	 break;
      case 'w':
	 fopen_mode |= __MODE_WRITE;
	 open_mode = (O_CREAT | O_TRUNC);
	 break;
      case 'a':
	 fopen_mode |= __MODE_WRITE;
	 open_mode = (O_CREAT | O_APPEND);
	 break;
      case '+':
	 fopen_mode |= __MODE_RDWR;
	 break;
#if __MODE_IOTRAN || O_BINARY
      case 'b':		/* Binary */
	 fopen_mode &= ~__MODE_IOTRAN;
	 open_mode |= O_BINARY;
#if __MODE_IOTRAN && !O_BINARY
	 do_iosense=0;
#endif
	 break;
      case 't':		/* Text */
	 fopen_mode |= __MODE_IOTRAN;
#if __MODE_IOTRAN && !O_BINARY
	 do_iosense=0;
#endif
	 break;
#endif
      }

   /* Add in the read/write options to mode for open() */
   switch (fopen_mode & (__MODE_READ | __MODE_WRITE))
   {
   case 0:
      return 0;
   case __MODE_READ:
      open_mode |= O_RDONLY;
      break;
   case __MODE_WRITE:
      open_mode |= O_WRONLY;
      break;
   default:
      open_mode |= O_RDWR;
      break;
   }

   if (!fname) {
        errno = EINVAL;
        return 0;
   }

   /* Allocate the (FILE) before we do anything irreversable */
   if (fp == 0)
   {
      nfp = malloc(sizeof(FILE));
      if (nfp == 0)
	 return 0;
   }

   /* Open the file itself */
   fd = open(fname, open_mode, 0666);
   if (fd < 0)			/* Grrrr */
   {
      if (nfp)
	 free(nfp);
      if (fp)
      {
	 fp->mode |= fopen_mode;
	 fclose(fp);			/* Deallocate if required */
      }
      return 0;
   }

   /* If this isn't freopen create a (FILE) and buffer for it */
   if (fp == 0)
   {
      fp = nfp;
      fp->next = __IO_list;
      __IO_list = fp;

      fp->mode = __MODE_FREEFIL;
      if( isatty(fd) )
      {
	 fp->mode |= _IOLBF;
#if __MODE_IOTRAN && !O_BINARY
	 if( do_iosense ) fopen_mode |= __MODE_IOTRAN;
#endif
      }
      else
	 fp->mode |= _IOFBF;
      fp->bufstart = malloc(BUFSIZ);
      if (fp->bufstart == 0)	/* Oops, no mem */
      {				/* Humm, full buffering with a two(!) byte
				 * buffer. */
	 fp->bufstart = fp->unbuf;
	 fp->bufend = fp->unbuf + sizeof(fp->unbuf);
      }
      else
      {
	 fp->bufend = fp->bufstart + BUFSIZ;
	 fp->mode |= __MODE_FREEBUF;
      }
   }

   /* Ok, file's ready clear the buffer and save important bits */
   fp->bufpos = fp->bufread = fp->bufwrite = fp->bufstart;
   fp->mode |= fopen_mode;
   fp->fd = fd;

   return fp;
}
