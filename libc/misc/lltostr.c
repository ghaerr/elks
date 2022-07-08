/* long long to string, Jul 2022 Greg Haerr */
#include <stdlib.h>

/* max long long = 9,223,372,036,854,775,807 */
char *lltostr(long long val, int radix)
{
   unsigned long long u = (val < 0)? 0u - val: val;
   char *p = ulltostr(u, radix);
   if(p && val < 0) *--p = '-';
   return p;
}
