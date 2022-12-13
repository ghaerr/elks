#include <string.h>

char * strncpy (char * d, const char * s, size_t l)
{
   char *s1=d;
   const char *s2=s;
   while(l > 0)
   {
      l--;
      if( (*s1++ = *s2++) == '\0')
         break;
   }

   /* This _is_ correct strncpy is supposed to zap */
   for(; l>0; l--) *s1++ = '\0';
   return d;
}
