/* Copyright (C) 1995,1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 * Apr 2020 Greg Haerr - made small as possible
 */
#include <stdlib.h>

char *ltostr(long val, int radix)
{
   unsigned long u = (val < 0)? 0u - val: val;
   char *p = ultostr(u, radix);
   if(p && val < 0) *--p = '-';
   return p;
}
