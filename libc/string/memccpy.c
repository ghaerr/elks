#ifdef L_memccpy
#include <string.h>

void *
memccpy(void * d, void * s, int c, size_t l)	/* Do we need a fast one ? */
{
   register char *s1=d, *s2=s;
   while (l-- > 0)
      if ((*s1++ = *s2++) == c )
         return s1;
   return 0;
}
#endif
