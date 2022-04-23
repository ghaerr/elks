/* Apr 2020 Greg Haerr - made small as possible */
#include <stdlib.h>
#define MAX_LONG_CHARS 34

char *ultostr(unsigned long val, int radix)
{
  static char buf[MAX_LONG_CHARS];
  char *p = buf + sizeof(buf) - 1;

  if (radix > 36 || radix < 2)
    return 0;

  *p = '\0';
  do {
      int c = val % radix;
      if (c > 9)
        *--p = 'a' - 10 + c;
      else
        *--p = '0' + c;
  } while ((val /= radix) != 0);
  return p;
}
