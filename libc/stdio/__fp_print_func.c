#ifdef L_fp_print
#include "_stdio.h"

#ifndef __HAS_NO_FLOATS__

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

void
__fp_print_func(double * pval, int style, int preci, char * ptmp)
{
   int decpt, negative;
   char * cvt;
   double val = *pval;

   if (preci < 0) preci = 6;

   cvt = fcvt(val, preci, &decpt, &negative);
   if(negative)
      *ptmp++ = '-';

   if (decpt<0) {
      *ptmp++ = '0';
      *ptmp++ = '.';
      while(decpt<0) {
	 *ptmp++ = '0'; decpt++;
      }
   }

   while(*cvt) {
      *ptmp++ = *cvt++;
      if (decpt == 1)
	 *ptmp++ = '.';
      decpt--;
   }

   while(decpt > 0) {
      *ptmp++ = '0';
      decpt--;
   }
}

static void
__xfpcvt()
{
   extern int (*__fp_print)();
   __fp_print = __fp_print_func;
}
#endif
#endif
