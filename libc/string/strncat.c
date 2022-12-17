#include <string.h>

char *
strncat(char *d, const char *s, size_t l)
{
   register char *s1=d+strlen(d), *s2;

   s2 = memchr(s, 0, l);
   if( s2 )
      memcpy(s1, s, s2-s+1);
   else
   {
      memcpy(s1, s, l);
      s1[l] = '\0';
   }
   return d;
}
