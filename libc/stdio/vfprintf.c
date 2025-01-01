/*
 *  vfprintf - A small implementation of printf-style format string processor
 *
 *          Only the following basic types are supported:
 *              %%      literal % sign
 *              %c      char
 *              %d/%i   signed decimal
 *              %u      unsigned decimal
 *              %o      octal
 *              %b      binary
 *              %s      string
 *              %x/%X   hexadecimal with lower/upper case letters
 *              %p      pointer - same as %04x
 *              %k      pticks (0.838usec intervals auto displayed as us, ms or s)
 *              %#k     pticks truncated at decimal point
 *              %efgEG  optional floating point formatting using dtostr
 *          The following flags preceding the format type are supported:
 *              0       fill with leading zeros
 *              1-9     minimum field width
 *              .       precision followed by 0-9 (strings only)
 *              -       left justifiy
 *              +       begin signed conversion with + or -
 *              SP      (space) replace + with space if not negative
 *              ,       thousands separator (can also use ' and _)
 *              l       long data
 *              h       short data
 *
 * This file originally based on printf.c from 'Dlibs' on the atari ST  (RdeBath)
 *
 * 19-OCT-88: Dale Schumacher
 * > John Stanley has again been a great help in debugging, particularly
 * > with the printf/scanf functions which are his creation.  
 *
 *    Dale Schumacher                         399 Beacon Ave.
 *    (alias: Dalnefre')                      St. Paul, MN  55104
 *    dal@syntel.UUCP                         United States of America
 *  "It's not reality that's important, but how you perceive things."
 *
 * Altered to use stdarg, made the core function vfprintf.
 * Hooked into the stdio package using 'inside information' -RDB
 *
 * Greg Haerr enhanced for speed, new features
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#ifndef __HAS_NO_FLOATS__
#include <sys/weaken.h>
/*
 * Use '#include <sys/weaken.h>` and '__STDIO_PRINT_FLOATS'
 * in user program to link in libc %e,%f,%g * printf/sprintf support
 * (see below, stdio.h and sys/weaken.h).
 */
#endif

/*
 * Output the given field in the manner specified by the arguments. Return
 * the number of characters output.
 */
static int
__fmt(FILE *op, unsigned char *buf, int ljustf, int width, int preci, char pad, char sign,
    int buffer_mode)
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
      putc(ch, op);
      if( ch == '\n' && buffer_mode == _IOLBF ) fflush(op);
   }

   return cnt;
}

int
vfprintf(FILE *op, const char *fmt, va_list ap)
{
   int i, cnt = 0, ljustf, lval;
   int preci, width, radix;
   unsigned int c;
   char pad, dpoint;
   char sign, quot;
   unsigned long v;
   int buffer_mode;
   char *p;
   int hash;
   char buf[64];

   /* turn off putc calling fputc every time for non or line buffered */
   buffer_mode = op->mode & __MODE_BUF;
   op->mode &= ~__MODE_BUF;

   while (*fmt) {
      if (*fmt == '%') {
         ljustf = 0;            /* left justify flag */
         hash = 0;              /* alternate output */
         quot = 0;              /* thousands grouping */
         dpoint = 0;            /* found decimal point */
         sign = '\0';           /* sign char & status */
         pad = ' ';             /* justification padding char */
         width = -1;            /* min field width */
         preci = -1;            /* max data width */
         radix = 10;            /* number base */
         p = buf;               /* pointer to area to print */
         lval = sizeof(int) == sizeof(long);
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

         case ' ':
         case '+':              /* leading sign flag */
            sign = *fmt;
            goto fmtnxt;

         case '\'':             /* thousands grouping */
         case ',':
         case '_':
            quot = *fmt;
            goto fmtnxt;

         case '#':
            hash = 1;
            goto fmtnxt;

         case '\0':             /* early EOS */
            continue;

         case '*':              /* parameter width value */
            i = va_arg(ap, int);
            if (dpoint)
               preci = i;
            else {
                if (i < 0) {
                    width = -i;
                    ljustf = 1;
                } else width = i;
            }
            goto fmtnxt;

         case '.':              /* secondary width field */
            dpoint = 1;
            goto fmtnxt;

         case 'l':              /* long data */
            lval = 1;
            goto fmtnxt;

         case 'h':              /* short data */
            lval = 0;
            goto fmtnxt;

         case 'b':              /* Unsigned binary */
            radix = 2;
            goto usproc;

         case 'o':              /* Unsigned octal */
            radix = 8;
            goto usproc;

         case 'p':              /* Pointer */
            if (sizeof(char *) == sizeof(long))
                lval = 1;
            width = lval? 8: 4;
            pad = '0';
            /* fall thru */

         case 'x':              /* Unsigned hexadecimal */
         case 'X':
            radix = 16;
            goto usproc;

         case 'd':              /* Signed decimal */
         case 'i':
            v = lval? va_arg(ap, long) : (long)va_arg(ap, int);
            if ((long)v < 0) {
                v = -(long)v;
                sign = '-';
            }
            goto convert;

         case 'u':              /* Unsigned decimal */
         case 'k':              /* Pticks */
          usproc:
            v = lval? va_arg(ap, unsigned long) : (unsigned long)va_arg(ap, unsigned int);
#ifndef __C86__
            if (*fmt == 'k') {
                if (_weakaddr(ptostr)) {
                    (_weakfn(ptostr))(v, hash, p);
                    preci = -1;
                    goto printit;
                }
                /* if precision timing not linked in, display as unsigned */
            }
#endif

        convert:
            p = buf + sizeof(buf) - 1;
            *p = '\0';
            for (i = 0;;) {
#ifdef _M_I86
                c = radix;
                v = __divmod(v, &c);    /* remainder returned in c */
#else
                c = v % radix;
                v = v / radix;
#endif
                if (c > 9)
                    *--p = ((*fmt == 'X')? 'A': 'a') - 10 + c;
                else
                    *--p = '0' + c;
                if (!v)
                    break;
                if (quot && ++i == 3) {
                    *--p = quot;
                    i = 0;
                }
            }
            //if (hash && radix == 8) {
                //width = strlen(p) + 1;
                //pad = '0';
            //}
            goto printit;

         case 'c':              /* Character */
            p[0] = va_arg(ap, int);
            p[1] = '\0';
            goto nopad;

         case 's':              /* String */
            p = va_arg(ap, char *);
            if (!p) p = "(null)";
          nopad:
            sign = '\0';
            pad = ' ';
          printit:
            cnt += __fmt(op, (unsigned char *)p, ljustf, width, preci, pad, sign,
                        buffer_mode);
            break;

#ifndef __HAS_NO_FLOATS__
         case 'e':              /* float */
         case 'f':
         case 'g':
         case 'E':
         case 'G':
            if (_weakaddr(dtostr)) {
               (_weakfn(dtostr))(va_arg(ap, double), *fmt, preci, p);
               preci = -1;
               goto printit;
            }
            /* fall thru if dotostr not linked in */
#endif

         default:               /* unknown character */
            goto charout;
         }
      } else {
       charout:
         putc(*fmt, op);        /* normal char out */
         if( *fmt == '\n' && buffer_mode == _IOLBF ) fflush(op);
         ++cnt;
      }
      ++fmt;
   }
   op->mode |= buffer_mode;
   if( buffer_mode == _IONBF ) fflush(op);
   if( buffer_mode == _IOLBF ) op->bufwrite = op->bufstart;
   return cnt;
}
