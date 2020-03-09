//-------------------------------------------------------------------------------

#include <string.h>

#include <asm/config.h>

#ifndef LIBC_ASM_MEMCPY

void * memcpy (void * d, const void * s, size_t l)
{
   register char *s1=d, *s2=(char *)s;
   for( ; l>0; l--) *((unsigned char*)s1++) = *((unsigned char*)s2++);
   return d;
}

#endif

//-------------------------------------------------------------------------------
