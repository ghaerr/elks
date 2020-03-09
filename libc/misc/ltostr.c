/* Copyright (C) 1995,1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */

#include <stdlib.h>

char *
ltostr (long val, int radix)
{
   char *p;
   int flg = 0;
   if( val < 0 ) { flg++; val= -val; }
   p = ultostr(val, radix);
   if(p && flg) *--p = '-';
   return p;
}
