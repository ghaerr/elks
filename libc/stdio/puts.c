#include <stdio.h>

int
puts(const char *s)
{
   register int n;

   if (((n = fputs(s, stdout)) == EOF)
       || (putc('\n', stdout) == EOF))
      return (EOF);
   return (++n);
}
