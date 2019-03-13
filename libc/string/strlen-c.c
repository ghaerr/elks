//-------------------------------------------------------------------------------

#include <string.h>

#include <asm/config.h>

#ifndef LIBC_ASM_STRLEN

size_t strlen (const char * str)
{
   register char * p = (char *) str;
   while (*p) p++;
   return p - str;
}

#endif

//-------------------------------------------------------------------------------
