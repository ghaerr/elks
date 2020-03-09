#include <string.h>

void *
memmove(void *d, void *s, size_t l)
{
   register char *s1=d, *s2=s;
   /* This bit of sneakyness c/o Glibc, it assumes the test is unsigned */
   if (s1-s2 >= l ) return memcpy(d,s,l);

   /* This reverse copy only used if we absolutly have to */
   s1+=l; s2+=l;
   while (l-- >0)
      *(--s1) = *(--s2);
   return d;
}
