//-------------------------------------------------------------------------------

#include <string.h>

#include <asm/config.h>

#ifndef LIBC_ASM_STRCPY

char * strcpy (char * dest, const char * src)
{
   /* This is probably the quickest on an 8086 but a CPU with a cache will
    * prefer to do this in one pass */
   return memcpy(d, s, strlen(s)+1);
}

#endif

//-------------------------------------------------------------------------------
