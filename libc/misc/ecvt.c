#ifndef __HAS_NO_FLOATS__

/* from dev86 libc */
#include <stdlib.h>

#define DIGMAX	30		/* max # of digits in string */
#define DIGPREC	17		/* max # of significant digits */
#define ECVT	0
#define FCVT	1
static char digstr[DIGMAX + 1 + 1];    /* +1 for end of string         */

    /* +1 in case rounding adds     */
    /* another digit                */
static double negtab[] =
    { 1e-256, 1e-128, 1e-64, 1e-32, 1e-16, 1e-8, 1e-4, 1e-2, 1e-1, 1.0 };
static double postab[] =
    { 1e+256, 1e+128, 1e+64, 1e+32, 1e+16, 1e+8, 1e+4, 1e+2, 1e+1 };

static char *
_cvt(int cnvflag, double val, int ndig, int *pdecpt, int *psign)
{
   int   decpt, pow, i;
   char *p;
   *psign = (val < 0) ? ((val = -val), 1) : 0;
   ndig = (ndig < 0) ? 0 : (ndig < DIGMAX) ? ndig : DIGMAX;
   if (val == 0) {
      for (p = &digstr[0]; p < &digstr[ndig]; p++)
	 *p = '0';
      decpt = 0;
   } else {
      /* Adjust things so that 1 <= val < 10  */
      /* in these loops if val == MAXDOUBLE)  */
      decpt = 1;
      pow = 256;
      i = 0;
      while (val < 1) {
	 while (val < negtab[i + 1]) {
	    val /= negtab[i];
	    decpt -= pow;
	 }
	 pow >>= 1;
	 i++;
      }
      pow = 256;
      i = 0;
      while (val >= 10) {
	 while (val >= postab[i]) {
	    val /= postab[i];
	    decpt += pow;
	 }
	 pow >>= 1;
	 i++;
      }
      if (cnvflag == FCVT) {
	 ndig += decpt;
	 ndig = (ndig < 0) ? 0 : (ndig < DIGMAX) ? ndig : DIGMAX;
      }

      /* Pick off digits 1 by 1 and stuff into digstr[]       */
      /* Do 1 extra digit for rounding purposes               */
      for (p = &digstr[0]; p <= &digstr[ndig]; p++) {
	 int   n;

	 /* 'twould be silly to have zillions of digits  */
	 /* when only DIGPREC are significant            */
	 if (p >= &digstr[DIGPREC])
	    *p = '0';

	 else {
	    n = val;
	    *p = n + '0';
	    val = (val - n) * 10;	/* get next digit */
	 }
      }
      if (*--p >= '5') {	/* if we need to round              */
	 while (1) {
	    if (p == &digstr[0]) {	/* if at start      */
	       ndig += cnvflag;
	       decpt++;		/* shift dec pnt */
	       digstr[0] = '1';	/* "100000..." */
	       break;
	    }
	    *p = '0';
	    --p;
	    if (*p != '9') {
	       (*p)++;
	       break;
	    }
	 }			/* while */
      }				/* if */
   }				/* else */
   *pdecpt = decpt;
   digstr[ndig] = 0;		/* terminate string             */
   return &digstr[0];
}

/*
 * Convert double val to a string of decimal digits.
 *	ndig = # of digits in resulting string
 * Returns:
 *	*pdecpt = position of decimal point from left of first digit
 *	*psign  = nonzero if value was negative
 */
char *
ecvt(double val, int ndig, int *pdecpt, int *psign)
{
   return _cvt(ECVT, val, ndig, pdecpt, psign);
}

char *
fcvt(double val, int nfrac, int *pdecpt, int *psign)
{
   return _cvt(FCVT, val, nfrac, pdecpt, psign);
}
#endif
