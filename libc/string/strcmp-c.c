#include <string.h>
#include <asm/config.h>

#ifndef LIBC_ASM_STRCMP

int
strcmp(const char *p1, const char *p2)
{
    const unsigned char *s1 = (const unsigned char *)p1;
    const unsigned char *s2 = (const unsigned char *)p2;
    unsigned char c1, c2;

    while ((c1 = *s1++) == (c2 = *s2++) && c1)
        continue;
    return c1 - c2;
}

#endif
