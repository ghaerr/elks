//-------------------------------------------------------------------------------

#include <string.h>

#include <asm/config.h>

#ifndef LIBC_ASM_MEMSET

void * memset (void * str, int c, size_t l)
{
   register char *s1=str;
   while (l-->0) *s1++ = c;
   return str;
}

#endif

//-------------------------------------------------------------------------------
