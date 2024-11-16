#include <string.h>

#ifdef BCC_ASM
  mov	bx,sp
  push	si
  mov	si,[bx+2]
  mov	bx,[bx+4]
  xor	ax,ax
  cld
in_loop:
  lodsb
  cmp	al,bl
  jz	got_it
  or	al,al
  jnz	in_loop
  pop	si
  ret
got_it:
  lea	ax,[si-1]
  pop	si
  ret
#endif

char *
strchr(const char * s, int c)
{
    for(; *s != (char)c; ++s) {
        if (*s == '\0')
            return NULL;
    }
    return (char *)s;
}

#ifdef __GNUC__
char *index(const char *s, int c) __attribute__ ((weak, alias ("strchr")));
#endif
