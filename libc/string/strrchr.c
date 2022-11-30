#include <string.h>

char *
strrchr (const char * s, int c)
{
   register const char * prev = 0;
   register const char * p = s;
   /* For null it's just like strlen */
   if( c == '\0' ) return (char *)p+strlen(p);

   /* everything else just step along the string. */
   while( (p=strchr(p, c)) != 0 )
   {
      prev = p; p++;
   }
   return (char *)prev;
}

#ifdef __GNUC__
char *rindex(const char *s, int c) __attribute__ ((weak, alias ("strrchr")));
#endif
