/* Copyright (C) 1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */

/* This is an implementation of the C standard IO package.
 */

#include <sys/types.h>

#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#ifndef O_BINARY
#define O_BINARY	0
#endif

extern FILE *__IO_list;		/* For fflush at exit */

#ifdef __AS386_16__
#define Inline_init
#endif

#ifdef __AS386_32__
#define Inline_init
#endif

#ifndef Inline_init
#define Inline_init __io_init_vars()
#endif

#define buferr (stderr->unbuf)	/* Stderr is unbuffered */

FILE *__IO_list = 0;		/* For fflush at exit */

static unsigned char bufin[BUFSIZ];
static unsigned char bufout[BUFSIZ];
#ifndef buferr
static unsigned char buferr[BUFSIZ];
#endif

FILE  stdin[1] =
{
   {bufin, bufin, bufin, bufin, bufin + sizeof(bufin),
    0, _IOFBF | __MODE_READ | __MODE_IOTRAN}
};

FILE  stdout[1] =
{
   {bufout, bufout, bufout, bufout, bufout + sizeof(bufout),
    1, _IOFBF | __MODE_WRITE | __MODE_IOTRAN}
};

FILE  stderr[1] =
{
   {buferr, buferr, buferr, buferr, buferr + sizeof(buferr),
    2, _IONBF | __MODE_WRITE | __MODE_IOTRAN}
};

/* Call the stdio initiliser; it's main job it to call atexit */

#ifdef __AS386_16__
#define STATIC static

#asm
  loc	1		! Make sure the pointer is in the correct segment
auto_func:		! Label for bcc -M to work.
  .word	___io_init_vars	! Pointer to the autorun function
  .text			! So the function after is also in the correct seg.
#endasm
#endif

#ifdef __AS386_32__
#define STATIC static

#asm
  loc	1		! Make sure the pointer is in the correct segment
auto_func:		! Label for bcc -M to work.
  .long	___io_init_vars	! Pointer to the autorun function
  .text			! So the function after is also in the correct seg.
#endasm
#endif

#ifndef STATIC
#define STATIC
#endif

STATIC void
__stdio_close_all()
{
   FILE *fp;
   fflush(stdout);
   fflush(stderr);
   for (fp = __IO_list; fp; fp = fp->next)
   {
      fflush(fp);
      close(fp->fd);
      /* Note we're not de-allocating the memory */
      /* There doesn't seem to be much point :-) */
      fp->fd = -1;
   }
}

STATIC void 
__io_init_vars()
{
#ifndef __AS386_16__
#ifndef __AS386_32__
   static int first_time = 1;
   if( !first_time ) return ;
   first_time = 0;
#endif
#endif
   if (isatty(1))
      stdout->mode |= _IOLBF;
   atexit(__stdio_close_all);
}

int fputc (int ch, FILE *fp)
{
   register int v;
   Inline_init;

   /* If last op was a read ... note fflush may change fp->mode and ret OK */
   if ((fp->mode & __MODE_READING) && fflush(fp))
      return EOF;

   v = fp->mode;
   /* Can't write if there's been an EOF or error then return EOF */
   if ((v & (__MODE_WRITE | __MODE_EOF | __MODE_ERR)) != __MODE_WRITE)
      return EOF;

   /* In MSDOS translation mode */
#if __MODE_IOTRAN && !O_BINARY
   if (ch == '\n' && (v & __MODE_IOTRAN) && fputc('\r', fp) == EOF)
      return EOF;
#endif

   /* Buffer is full */
   if (fp->bufpos >= fp->bufend && fflush(fp))
      return EOF;

   /* Right! Do it! */
   *(fp->bufpos++) = ch;
   fp->mode |= __MODE_WRITING;

   /* Unbuffered or Line buffered and end of line */
   if (((ch == '\n' && (v & _IOLBF)) || (v & _IONBF))
       && fflush(fp))
      return EOF;

   /* Can the macro handle this by itself ? */
   if (v & (__MODE_IOTRAN | _IOLBF | _IONBF))
      fp->bufwrite = fp->bufstart;	/* Nope */
   else
      fp->bufwrite = fp->bufend;	/* Yup */

   /* Correct return val */
   return (unsigned char) ch;
}

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

int fflush(FILE *fp)
{
   int   len, cc, rv=0;
   unsigned char * bstart;
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

/* Nothing special here ... */
char *fgets(char *s, size_t count, FILE *f)
{
   char *ret;
   register size_t i;
   register int ch;

   ret = s;
   for (i = count-1; i > 0; i--)
   {
      ch = getc(f);
      if (ch == EOF)
      {
	 if (s == ret)
	    return 0;
	 break;
      }
      *s++ = (char) ch;
      if (ch == '\n')
	 break;
   }
   *s = 0;

   if (ferror(f))
      return 0;
   return ret;
}

char *
gets(str)			/* BAD function; DON'T use it! */
char *str;
{
   /* Auwlright it will work but of course _your_ program will crash */
   /* if it's given a too long line */
   register char *p = str;
   register int c;

   while (((c = getc(stdin)) != EOF) && (c != '\n'))
      *p++ = c;
   *p = '\0';
   return (((c == EOF) && (p == str)) ? NULL : str);	/* NULL == EOF */
}

int fputs(const char *str, FILE *fp)
{
   register int n = 0;
   while (*str)
   {
      if (putc(*str++, fp) == EOF)
	 return (EOF);
      ++n;
   }
   return (n);
}

int puts (const char * s)
{
   register int n;

   if (((n = fputs(s, stdout)) == EOF)
       || (putc('\n', stdout) == EOF))
      return (EOF);
   return (++n);
}

/*
 * fread will often be used to read in large chunks of data calling read()
 * directly can be a big win in this case. Beware also fgetc calls this
 * function to fill the buffer.
 * 
 * This ignores __MODE__IOTRAN; probably exactly what you want. (It _is_ what
 * fgetc wants)
 */
size_t fread(void *buf, size_t size, size_t nelm, FILE *fp)
{
   size_t len;
   int v;
   size_t bytes, got = 0;
   Inline_init;

   v = fp->mode;

   /* Want to do this to bring the file pointer up to date */
   if (v & __MODE_WRITING)
      fflush(fp);

   /* Can't read or there's been an EOF or error then return zero */
   if ((v & (__MODE_READ | __MODE_EOF | __MODE_ERR)) != __MODE_READ)
      return 0;

   /* This could be long, doesn't seem much point tho */
   bytes = size * nelm;

   len = fp->bufread - fp->bufpos;
   if (len >= bytes)		/* Enough buffered */
   {
      memcpy(buf, fp->bufpos, bytes);
      fp->bufpos += bytes;
      return nelm;
   }
   else if (len > 0)		/* Some buffered */
   {
      memcpy(buf, fp->bufpos, len);
      fp->bufpos += len;
      got = len;
   }

   /* Need more; do it with a direct read */
   len = read(fp->fd, (char *)buf + got, bytes - got);

   /* Possibly for now _or_ later */
   if (len < 0)
   {
      fp->mode |= __MODE_ERR;
      len = 0;
   }
   else if (len == 0)
      fp->mode |= __MODE_EOF;

   return (got + len) / size;
}

/*
 * Like fread, fwrite will often be used to write out large chunks of
 * data; calling write() directly can be a big win in this case.
 * 
 * But first we check to see if there's space in the buffer.
 * 
 * Again this ignores __MODE__IOTRAN.
 */
int fwrite(char *buf, int size, int nelm, FILE *fp)
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

#ifdef L_rewind
void
rewind(fp)
FILE * fp;
{
   fseek(fp, (long)0, 0);
   clearerr(fp);
}
#endif

int
fseek(fp, offset, ref)
FILE *fp;
long  offset;
int   ref;
{
#if 1
   /* if __MODE_READING and no ungetc ever done can just move pointer */

   if ( (fp->mode &(__MODE_READING | __MODE_UNGOT)) == __MODE_READING && 
        ( ref == SEEK_SET || ref == SEEK_CUR ))
   {
      long fpos = lseek(fp->fd, 0L, SEEK_CUR);
      if( fpos == -1 ) return EOF;

      if( ref == SEEK_CUR )
      {
         ref = SEEK_SET;
	 offset = fpos + offset + fp->bufpos - fp->bufread;
      }
      if( ref == SEEK_SET )
      {
         if ( offset < fpos && offset >= fpos + fp->bufstart - fp->bufread )
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

#ifdef L_ftell
long ftell(fp)
FILE * fp;
{
   long rv;
   if (fflush(fp) == EOF)
      return EOF;
   return lseek(fp->fd, 0L, SEEK_CUR);
}
#endif

//-----------------------------------------------------------------------------

FILE * fopen (char * file, char * mode)
{
	return __fopen(file, -1, (FILE*)0, mode);
}

//-----------------------------------------------------------------------------

FILE *
freopen(file, mode, fp)
char * file;
char * mode;
FILE * fp;
{
   return __fopen(file, -1, fp, mode);
}

//-----------------------------------------------------------------------------

FILE *
fdopen(file, mode)
int file;
char * mode;
{
   return __fopen((char*)0, file, (FILE*)0, mode);
}

//-----------------------------------------------------------------------------

FILE * __fopen (char * fname, int fd, FILE * fp, char * mode)
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

   /* Allocate the (FILE) before we do anything irreversable */
   if (fp == 0)
   {
      nfp = malloc(sizeof(FILE));
      if (nfp == 0)
	 return 0;
   }

   /* Open the file itself */
   if (fname)
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

//-----------------------------------------------------------------------------

int fclose (FILE *fp)
{
   int   rv = 0;

   if (fp == 0)
   {
      errno = EINVAL;
      return EOF;
   }
   if (fp->fd != -1)
   {
      if (fflush(fp))
	 return EOF;

      if (close(fp->fd))
	 rv = EOF;
      fp->fd = -1;
   }

   if (fp->mode & __MODE_FREEBUF)
   {
      free(fp->bufstart);
      fp->mode &= ~__MODE_FREEBUF;
      fp->bufstart = fp->bufend = 0;
   }

   if (fp->mode & __MODE_FREEFIL)
   {
      FILE *prev = 0, *ptr;
      fp->mode = 0;

      for (ptr = __IO_list; ptr && ptr != fp; ptr = ptr->next)
	 ;
      if (ptr == fp)
      {
	 if (prev == 0)
	    __IO_list = fp->next;
	 else
	    prev->next = fp->next;
      }
      free(fp);
   }
   else
      fp->mode = 0;

   return rv;
}

void
setbuffer(fp, buf, size)
FILE * fp;
char * buf;
int size;
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

int setvbuf(fp, buf, mode, size)
FILE * fp;
char * buf;
int mode;
size_t size;
{
   int rv = 0;
   fflush(fp);
   if( fp->mode & __MODE_FREEBUF ) free(fp->bufstart);
   fp->mode &= ~(__MODE_FREEBUF|__MODE_BUF);
   fp->bufstart = fp->unbuf;
   fp->bufend = fp->unbuf + sizeof(fp->unbuf);
   fp->mode |= _IONBF;

   if( mode == _IOFBF || mode == _IOLBF )
   {
      if( size <= 0  ) size = BUFSIZ;
      if( buf == 0 )
      {
         if( (buf = malloc(size)) != 0 )
	    fp->mode |= __MODE_FREEBUF;
         else rv = EOF;
      }
      if( buf )
      {
         fp->bufstart = (unsigned char *)buf;
         fp->bufend = (unsigned char *)buf+size;
         fp->mode &= ~__MODE_BUF;
         fp->mode |= mode;
      }
   }
   fp->bufpos = fp->bufread = fp->bufwrite = fp->bufstart;
   return rv;
}

//-----------------------------------------------------------------------------

int ungetc (int c, FILE * fp)
{
   if (fp->mode & __MODE_WRITING)
      fflush(fp);

   /* Can't read or there's been an error then return EOF */
   if ((fp->mode & (__MODE_READ | __MODE_ERR)) != __MODE_READ)
      return EOF;

   /* Can't do fast fseeks */
   fp->mode |= __MODE_UNGOT;

   if( fp->bufpos > fp->bufstart )
      return *--fp->bufpos = (unsigned char) c;
   else if( fp->bufread == fp->bufstart )
      return *fp->bufread++ = (unsigned char) c;
   else
      return EOF;
}

//-----------------------------------------------------------------------------
