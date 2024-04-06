/*
 * Tiny vfprintf - based on ELKS stdio
 *
 * Reduces executable size when linked with app for programs requiring
 *  output to terminal only (stdout, stderr) and no file I/O.
 * Automatically usable with:
 *  printf, fprintf, sprintf
 *
 * Limitations:
 *	%s, %c, %d, %u, %x, %o, %ld, %lu, %lx, %lo only w/field width & precision
 *  Don't use with fopen (stdout, stderr only)
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

/*
 * Output the given field in the manner specified by the arguments. Return
 * the number of characters output.
 */
static int
prtfld(FILE *op, unsigned char *buf, int ljustf, char pad, int width, int preci)
{
   int cnt = 0, len;
   unsigned char ch;

   len = strlen((char *)buf);

   if ((preci != -1) && (len > preci))  /* limit max data width */
      len = preci;

   if (width < len)             /* flexible field width or width overflow */
      width = len;

   /*
    * at this point: width = total field width len   = actual data width
    */
   cnt = width;
   width -= len;

   while (width || len)
   {
      if (!ljustf && width)     /* left padding */
      {
         ch = pad;
         --width;
      }
      else if (len)
      {
         ch = *buf++;        /* main field */
         --len;
      }
      else
      {
         ch = pad;              /* right padding */
         --width;
      }
      __fputc(ch, op);
   }

   return cnt;
}

int vfprintf(FILE *op, const char *fmt, va_list ap)
{
   int cnt = 0;
   int i, width, preci, radix;
   int ljustf, lval, pad, dpoint;
   char *ptmp;
   char  tmp[64];

   while (*fmt)
   {
      if (*fmt == '%')
      {
	 ljustf = 0;		/* left justify flag */
	 dpoint = 0;		/* found decimal point */
	 lval = 0;
	 width = -1;		/* min field width */
	 preci = -1;		/* max data width */
	 pad = ' ';		/* justification padding char */
	 radix = 10;		/* number base */
	 ptmp = tmp;		/* pointer to area to print */
       fmtnxt:
	 i = 0;
	 for(;;)
	 {
	    ++fmt;
	    if(*fmt < '0' || *fmt > '9' ) break;
	    i = (i * 10) + (*fmt - '0');
	    if (dpoint)
	       preci = i;
	    else if (!i && (pad == ' '))
	    {
	       pad = '0';
	       goto fmtnxt;
	    }
	    else
	       width = i;
	 }

	 switch (*fmt)
	 {
	 case '-':		/* left justification */
	    ljustf = 1;
	    goto fmtnxt;

	 case '.':		/* secondary width field */
	    dpoint = 1;
	    goto fmtnxt;

	 case 'l':		/* long data */
	    lval = 1;
	 case 'h':      /* short data */
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
	    cnt += prtfld(op, (unsigned char *)ptmp, ljustf, pad, width, preci);
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
