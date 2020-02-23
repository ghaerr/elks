#include <string.h>

void *
memchr(const void * str, int c, size_t l)
{
#ifdef BCC_ASM
#asm
  mov	bx,sp
  push	di

#ifdef PARANOID
  push	es
  push	ds	; Im not sure if this is needed, so just in case.
  pop	es
  cld
#endif

  mov	di,[bx+2]
  mov	ax,[bx+4]
  mov	cx,[bx+6]
  test	cx,cx
  je	is_z	! Zero length, do not find.

  repne		! Scan
  scasb
  jne	is_z	! Not found, ret zero
  dec	di	! Adjust ptr
  mov	ax,di	! return
  jmp	xit
is_z:
  xor	ax,ax
xit:

#ifdef PARANOID
  pop	es
#endif
  pop	di
#endasm
#else /* ifdef BCC_ASM */
   register char *p=(char *)str;
   while(l-- > 0)
   {
      if(*p == c) return p;
      p++;
   }
   return 0;
#endif /* ifdef BCC_ASM */
}
