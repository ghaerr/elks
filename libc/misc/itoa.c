/* Apr 2020 Greg Haerr - handles INT_MIN */
#include <stdlib.h>
#define MAX_INT_CHARS 7

char *itoa(int val)
{
   static char buf[MAX_INT_CHARS];
   char *b = buf + sizeof(buf) - 1;
   unsigned int u = (val < 0)? 0u - val: val;

   *b = '\0';
   do {
      *--b = '0' + (u % 10);
   } while ((u /= 10) != 0);
   if (val < 0)
      *--b = '-';
   return b;
}
