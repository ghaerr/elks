/*
 * Tiny vfprintf - based on ELKS stdio
 *
 * Reduces executable size when linked with app.
 * Automatically usable with:
 *  printf, fprintf, sprintf
 *
 * Limitations:
 *	%s, %c, %d, %u, %x, %o, %ld, %lu, %lx only
 *  No field widths
 *    Replaces stdout and stderr buffers with single buffer
 *
 * Mar 2020 Greg Haerr
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>

static unsigned char bufout[80];

FILE  stdout[1] =
{
   {
    bufout,
    bufout,
    bufout,
    bufout,
    bufout + sizeof(bufout),
    1,
    _IOFBF | __MODE_WRITE | __MODE_IOTRAN
   }
};

FILE  stderr[1] =
{
   {
    bufout,
    bufout,
    bufout,
    bufout,
    bufout + sizeof(bufout),
    2,
    _IOFBF | __MODE_WRITE | __MODE_IOTRAN
   }
};

static void __fflush(FILE *fp)
{
   int   len;

   /* Return if this is a fake FILE from sprintf */
   if (fp->fd < 0)
      return;

   len = fp->bufpos - fp->bufstart;
   if (len)
      write(fp->fd, fp->bufstart, len);

   fp->bufwrite = fp->bufpos = fp->bufstart;
}

static void __fputc(int ch, FILE *fp)
{
   if (fp->bufpos >= fp->bufend)
     __fflush(fp);

   *(fp->bufpos++) = ch;

   fp->bufwrite = fp->bufend;
}

int vfprintf(FILE *op, const char *fmt, va_list ap)
{
   int cnt = 0;
   int lval;
   int radix;
   char *ptmp;
   char  tmp[64];

   while (*fmt)
   {
      if (*fmt == '%')
      {
	 radix = 10;		/* number base */
	 ptmp = tmp;		/* pointer to area to print */
	 lval = 0;
       fmtnxt:
	 ++fmt;

	 switch (*fmt)
	 {
	 case 'l':		/* long data */
	    lval = 1;
	    goto fmtnxt;

	 case 'd':		/* Signed decimal */
	    ptmp = ltostr((long) ((lval)
			 ? va_arg(ap, long)
			 : va_arg(ap, int)), 10);
	    goto printit;

	 case 'o':		/* Unsigned octal */
	    radix = 8;
	    goto usproc;

     case 'x':      /* Unsigned hexadecimal */
        radix = 16;
        /* fall thru */

	 case 'u':		/* Unsigned decimal */
	  usproc:
	    ptmp = ultostr((unsigned long) ((lval)
			? va_arg(ap, unsigned long)
			: va_arg(ap, unsigned int)), radix);
	    goto printit;

	 case 'c':		/* Character */
	    ptmp[0] = va_arg(ap, int);
	    ptmp[1] = '\0';
	    goto nopad;

	 case 's':		/* String */
	    ptmp = va_arg(ap, char*);
	  nopad:
	  printit:
	    do {
	    	__fputc(*ptmp++, op);
		cnt++;
	    } while (*ptmp);
	    break;

	 default:		/* unknown character */
	    goto charout;
	 }
      }
      else
      {
       charout:
	 __fputc(*fmt, op);	/* normal char out */
	 ++cnt;
      }
      ++fmt;
   }
   __fflush(op);
   return cnt;
}
