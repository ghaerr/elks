#ifndef __HAS_NO_FLOATS__

/*
 * floating point output
 *
 * Added 'g' output format
 * Apr 2022 Greg Haerr
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

void dtostr(double val, int style, int preci, char *buf)
{
   int decpt, negative, diddecpt = 0;
   char * cvt;

   style = tolower(style);
   if (preci < 0) preci = 6;

   if (style == 'e')
       cvt = ecvt(val, preci, &decpt, &negative);	/* preci = ndigits */
   else cvt = fcvt(val, preci, &decpt, &negative);
   if(negative)
      *buf++ = '-';

   if (decpt<=0) {
      *buf++ = '0';
      *buf++ = '.';
      diddecpt = 1;
      while(decpt<0) {
         *buf++ = '0';
         decpt++;
      }
   }

   while(*cvt) {
      *buf++ = *cvt++;
      if (decpt == 1) {
         *buf++ = '.';
         diddecpt = 1;
      }
      decpt--;
   }

   while(decpt > 0) {
      *buf++ = '0';
      decpt--;
   }
   *buf = 0;
   if (style == 'g' && diddecpt) {
      for (;;) {
         int c = *--buf;
         if (c == '0' || c == '.')
            *buf = 0;
         if (c != '0') break;
      }
   }

}

#if 0
static void
__xfpcvt()
{
   extern int (*__fp_print)();
   __fp_print = __fp_print_func;
}

#ifdef __AS386_16__
#asm
  loc   1         ! Make sure the pointer is in the correct segment
auto_func:        ! Label for bcc -M to work.
  .word ___xfpcvt ! Pointer to the autorun function
  .text           ! So the function after is also in the correct seg.
#endasm
#endif

#ifdef __AS386_32__
#asm
  loc   1         ! Make sure the pointer is in the correct segment
auto_func:        ! Label for bcc -M to work.
  .long ___xfpcvt ! Pointer to the autorun function
  .text           ! So the function after is also in the correct seg.
#endasm
#endif
#endif /* #if 0 */

#endif /* __HAS_NO_FLOATS__ */
