#include <string.h>
#include <asm/config.h>

#ifndef LIBC_ASM_FMEMSET

void __far *fmemset(void __far *str, int c, size_t l)
{
    char __far *s1 = str;

    while (l-- > 0)
        *s1++ = c;
    return str;
}

#endif
