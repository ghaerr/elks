#include <string.h>

int
memcmp(const void *s, const void *d, size_t l)
{
#ifdef BCC_ASM
#asm
  mov	bx,sp
  push	di
  push	si

#ifdef PARANOID
  push	es
  push	ds	! Im not sure if this is needed, so just in case.
  pop	es
  cld
#endif

  mov	si,[bx+2]	! Fetch
  mov	di,[bx+4]
  mov	cx,[bx+6]
  xor	ax,ax

  rep			! Bzzzzz
  cmpsb
  je	xit		! All the same!
  sbb	ax,ax
  sbb	ax,#-1		! choose +/-1
xit:
#ifdef PARANOID
  pop	es
#endif
  pop	si
  pop	di
#endasm
#else /* ifdef BCC_ASM */
   register const char *s1=d, *s2=s;
   register char c1=0, c2=0;
   while (l-- > 0)
      if ((c1= *s1++) != (c2= *s2++) )
         break;
   return c1-c2;
#endif /* ifdef BCC_ASM */
}
