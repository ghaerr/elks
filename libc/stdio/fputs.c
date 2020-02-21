#include <stdio.h>

int
fputs(const char *str, FILE *fp)
{
   register int n = 0;
   while (*str)
   {
      if (putc(*str++, fp) == EOF)
	 return (EOF);
      ++n;
   }
   return (n);
}
