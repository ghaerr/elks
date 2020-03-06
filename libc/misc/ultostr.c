#include <stdlib.h>

char *
ultostr (unsigned long val, int radix)
{
  static char buf[34];
  register char *p;
  register int c;

  if (radix > 36 || radix < 2)
    return 0;

  p = buf + sizeof (buf);
  *--p = '\0';

  do
    {
      c = val % radix;
      val /= radix;
      if (c > 9)
	*--p = 'a' - 10 + c;
      else
	*--p = '0' + c;
    }
  while (val);
  return p;
}
