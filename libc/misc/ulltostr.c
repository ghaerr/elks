/* unsigned long long to string, Jul 2022 Greg Haerr */
#include <stdlib.h>

#define MAX_LONG_LONG_CHARS 66

char *ulltostr(unsigned long long val, int radix)
{
  static char buf[MAX_LONG_LONG_CHARS];
  char *p = buf + sizeof(buf) - 1;

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
