/*
 * This file lifted in toto from 'Dlibs' on the atari ST  (RdeBath)
 *
 * 
 *    Dale Schumacher                         399 Beacon Ave.
 *    (alias: Dalnefre')                      St. Paul, MN  55104
 *    dal@syntel.UUCP                         United States of America
 *  "It's not reality that's important, but how you perceive things."
 */

/*
 * Sun Feb  8 21:02:15 EST 1998 claudio@pos.inf.ufpr.br (Claudio Matsuoka)
 * Changed sort direction
 */

#include <string.h>

char *_qbuf = 0;		/* pointer to storage for qsort() */

#define	PIVOT			((i+j)>>1)
#define moveitem(dst,src,size)	if(dst != src) memcpy(dst, src, size)

static 
_wqsort(base, lo, hi, cmp)
register int *base;
register int lo;
register int hi;
register int (*cmp) ();
{
   int   k;
   register int i, j, t;
   register int *p = &k;

   while (hi > lo)
   {
      i = lo;
      j = hi;
      t = PIVOT;
      *p = base[t];
      base[t] = base[i];
      base[i] = *p;
      while (i < j)
      {
	 while (((*cmp) ((base + j), p)) <= 0)
	    --j;
	 base[i] = base[j];
	 while ((i < j) && (((*cmp) ((base + i), p)) > 0))
	    ++i;
	 base[j] = base[i];
      }
      base[i] = *p;
      if ((i - lo) < (hi - i))
      {
	 _wqsort(base, lo, (i - 1), cmp);
	 lo = i + 1;
      }
      else
      {
	 _wqsort(base, (i + 1), hi, cmp);
	 hi = i - 1;
      }
   }
}

static 
_lqsort(base, lo, hi, cmp)
register long *base;
register int lo;
register int hi;
register int (*cmp) ();
{
   long  k;
   register int i, j, t;
   register long *p = &k;

   while (hi > lo)
   {
      i = lo;
      j = hi;
      t = PIVOT;
      *p = base[t];
      base[t] = base[i];
      base[i] = *p;
      while (i < j)
      {
	 while (((*cmp) ((base + j), p)) <= 0)
	    --j;
	 base[i] = base[j];
	 while ((i < j) && (((*cmp) ((base + i), p)) > 0))
	    ++i;
	 base[j] = base[i];
      }
      base[i] = *p;
      if ((i - lo) < (hi - i))
      {
	 _lqsort(base, lo, (i - 1), cmp);
	 lo = i + 1;
      }
      else
      {
	 _lqsort(base, (i + 1), hi, cmp);
	 hi = i - 1;
      }
   }
}

static 
_nqsort(base, lo, hi, size, cmp)
register char *base;
register int lo;
register int hi;
register int size;
register int (*cmp) ();
{
   register int i, j;
   register char *p = _qbuf;

   while (hi > lo)
   {
      i = lo;
      j = hi;
      p = (base + size * PIVOT);
      moveitem(_qbuf, p, size);
      moveitem(p, (base + size * i), size);
      moveitem((base + size * i), _qbuf, size);
      p = _qbuf;
      while (i < j)
      {
	 while (((*cmp) ((base + size * j), p)) <= 0)
	    --j;
	 moveitem((base + size * i), (base + size * j), size);
	 while ((i < j) && (((*cmp) ((base + size * i), p)) > 0))
	    ++i;
	 moveitem((base + size * j), (base + size * i), size);
      }
      moveitem((base + size * i), p, size);
      if ((i - lo) < (hi - i))
      {
	 _nqsort(base, lo, (i - 1), size, cmp);
	 lo = i + 1;
      }
      else
      {
	 _nqsort(base, (i + 1), hi, size, cmp);
	 hi = i - 1;
      }
   }
}

qsort(base, num, size, cmp)
char *base;
int   num;
int   size;
int   (*cmp) ();
{
   char  _qtemp[128];

   if (_qbuf == 0)
   {
      if (size > sizeof(_qtemp))/* records too large! */
	 return;
      _qbuf = _qtemp;
   }
   if (size == 2)
      _wqsort(base, 0, num - 1, cmp);
   else if (size == 4)
      _lqsort(base, 0, num - 1, cmp);
   else
      _nqsort(base, 0, num - 1, size, cmp);
   if (_qbuf == _qtemp)
      _qbuf = 0;
}
