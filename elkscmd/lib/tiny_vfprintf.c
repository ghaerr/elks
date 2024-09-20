/*
 * Tiny vfprintf - based on ELKS stdio
 *
 * Reduces executable size when linked with app for programs requiring
 *  output to terminal only (stdout, stderr) and no file I/O.
 * Automatically usable with:
 *  printf, fprintf, sprintf, fputc, fflush
 *
 * Limitations:
 *      %s, %c, %d, %u, %x, %o, %ld, %lu, %lx, %lo only w/field width & precision
 *  Don't use with fopen (stdout, stderr only)
 *  Always line buffered
 *
 * Mar 2020 Greg Haerr
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/rtinit.h>

static unsigned char bufout[80];
static unsigned char buferr[80];

FILE  stdout[1] =
{
   {
    bufout,
    bufout,
    bufout + sizeof(bufout),    /* putc is full buffered */
    bufout,
    bufout + sizeof(bufout),
    1,
    _IOLBF | __MODE_WRITE | __MODE_IOTRAN
   }
};

FILE  stderr[1] =
{
   {
    buferr,
    buferr,
    buferr + sizeof(buferr),    /* putc is full buffered */
    buferr,
    buferr + sizeof(buferr),
    2,
    _IOLBF | __MODE_WRITE | __MODE_IOTRAN
   }
};

/* name clash with stdio/init.c if __stdio_fini name used */
#pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
DESTRUCTOR(__exit_flush, _INIT_PRI_STDIO);
void __exit_flush(void)
{
   fflush(stdout);
   fflush(stderr);
}

int fflush(FILE *fp)
{
   int len;

   if (fp->fd < 0)              /* Return if this is a fake FILE from sprintf */
      return EOF;

   len = fp->bufpos - fp->bufstart;
   if (len)
      write(fp->fd, fp->bufstart, len);

   fp->bufpos = fp->bufstart;
   return 0;
}

int fputc(int ch, FILE *fp)
{
   if (fp->bufpos >= fp->bufend)
     fflush(fp);

   *(fp->bufpos++) = ch;

   if (ch == '\n')              /* fputc is always line buffered */
     fflush(fp);
    return ch;
}

/*
 * Output the given field in the manner specified by the arguments. Return
 * the number of characters output.
 */
static int
__fmt(FILE *op, unsigned char *buf, int ljustf, int width, int preci, char pad, char sign)
{
   int cnt = 0, len;
   unsigned char ch;

   len = strlen((char *)buf);

   if (*buf == '-')
      sign = *buf++;
   else if (sign)
      len++;


   if (preci != -1 && len > preci)  /* limit max data width */
      len = preci;

   if (width < len)             /* flexible field width or width overflow */
      width = len;

   /*
    * at this point: width = total field width, len = actual data width
    * (including possible sign character)
    */
   cnt = width;
   width -= len;

   while (width || len)
   {
      if (!ljustf && width)     /* left padding */
      {
         if (len && sign && pad == '0')
            goto showsign;
         ch = pad;
         --width;
      }
      else if (len)
      {
         if (sign)
         {
      showsign:
            ch = sign;          /* sign */
            sign = '\0';
         }
         else
            ch = *buf++;        /* main field */
         --len;
      }
      else
      {
         ch = pad;              /* right padding */
         --width;
      }
      fputc(ch, op);
   }

   return cnt;
}

int
vfprintf(FILE *op, const char *fmt, va_list ap)
{
   int i, cnt = 0, ljustf, lval;
   int preci, width, radix;
   unsigned int c;
   unsigned long v;
   char pad, dpoint;
   char *p;
   char sign;
   char buf[64];

   while (*fmt) {
      if (*fmt == '%') {
         ljustf = 0;            /* left justify flag */
         dpoint = 0;            /* found decimal point */
         sign = '\0';           /* sign char & status */
         lval = 0;
         pad = ' ';             /* justification padding char */
         width = -1;            /* min field width */
         preci = -1;            /* max data width */
         radix = 10;            /* number base */
         p = buf;               /* pointer to area to print */
       fmtnxt:
         i = 0;
         for (;;) {
            ++fmt;
            if (*fmt < '0' || *fmt > '9')
                break;
            i = i * 10 + *fmt - '0';
            if (dpoint)
               preci = i;
            else if (!i && pad == ' ') {
               pad = '0';
               goto fmtnxt;
            }
            else
               width = i;
         }

         switch (*fmt) {
         case '-':              /* left justification */
            ljustf = 1;
            goto fmtnxt;

         case '.':              /* secondary width field */
            dpoint = 1;
            goto fmtnxt;

         case 'l':              /* long data */
            lval = 1;
         case 'h':              /* short data */
            goto fmtnxt;

         case 'o':              /* Unsigned octal */
            radix = 8;
            goto usproc;

         case 'x':              /* Unsigned hexadecimal */
            radix = 16;
            goto usproc;

         case 'd':              /* Signed decimal */
            v = lval? va_arg(ap, long) : (long)va_arg(ap, int);
            if ((long)v < 0) {
                v = -(long)v;
                sign = '-';
            }
            goto convert;

         case 'u':              /* Unsigned decimal */
          usproc:
            v = lval? va_arg(ap, unsigned long) : (unsigned long)va_arg(ap, unsigned int);
        convert:
            p = buf + sizeof(buf) - 1;
            *p = '\0';
            do {
                c = radix;
                v = __divmod(v, &c);    /* remainder returned in c */
                if (c > 9)
                    *--p = 'A' - 10 + c;
                else
                    *--p = '0' + c;
            } while (v != 0);
            goto printit;

         case 'c':              /* Character */
            p[0] = va_arg(ap, int);
            p[1] = '\0';
            goto nopad;

         case 's':              /* String */
            p = va_arg(ap, char *);
          nopad:
            sign = '\0';
            pad = ' ';
          printit:
            cnt += __fmt(op, (unsigned char *)p, ljustf, width, preci, pad, sign);
            break;

         default:               /* unknown character */
            goto charout;
         }
      } else {
       charout:
         fputc(*fmt, op);     /* normal char out */
         ++cnt;
      }
      ++fmt;
   }
   return cnt;
}
