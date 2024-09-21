/* Apr 2020 Greg Haerr - handles LONG_MIN */
#include <stdlib.h>
#define MAX_LONG_CHARS 34

char *ltoa(long val)
{
   static char buf[MAX_LONG_CHARS];
   char *b = buf + sizeof(buf) - 1;
   unsigned long u = (val < 0)? 0u - val: val;

   *b = '\0';
#ifdef _M_I86
    do {
        unsigned int c;
        c = 10;
        u = __divmod(u, &c);
        *--b = '0' + c;
    } while (u);
#else
   do {
      *--b = '0' + (u % 10);
   } while ((u /= 10) != 0);
#endif
   if (val < 0)
      *--b = '-';
   return b;
}
