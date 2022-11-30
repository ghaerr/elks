#include <string.h>

char *
strchr (const char * s, int c)
{
#ifdef BCC_AX_ASM
#asm
  mov	bx,sp
  push	si
#if __FIRST_ARG_IN_AX__
  mov	bx,[bx+2]
  mov	si,ax
#else
  mov	si,[bx+2]
  mov	bx,[bx+4]
#endif
  xor	ax,ax

#ifdef PARANOID
  cld
#endif

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

#endasm
#else /* ifdef BCC_AX_ASM */
    for(; *s != (char)c; ++s) {
        if (*s == '\0')
            return NULL;
    }
    return (char *)s;
#endif /* ifdef BCC_AX_ASM */
}

#ifdef __GNUC__
char *index(const char *s, int c) __attribute__ ((weak, alias ("strchr")));
#endif
