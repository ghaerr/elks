/* Apr 2020 Greg Haerr - made small as possible */
#include <stdlib.h>
#define MAX_LONG_CHARS 34

char *ultostr(unsigned long val, int radix)
{
  static char buf[MAX_LONG_CHARS];
  char *p = buf + sizeof(buf) - 1;

  *p = '\0';
  do {
#ifdef _M_I86
      unsigned int c;
      c = radix;
      val = __divmod(val, &c);
#else
      unsigned int c = val % radix;
      val = val / radix;
#endif
      if (c > 9)
        *--p = 'a' - 10 + c;
      else
        *--p = '0' + c;
  } while (val);
  return p;
}
