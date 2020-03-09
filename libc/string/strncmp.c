#include <string.h>

int
strncmp(const char * d, const char * s, size_t l)
{
#ifdef BCC_AX_ASM
#asm
  mov	bx,sp
  push	si
  push	di

#ifdef PARANOID
  push	es
  push	ds	! Im not sure if this is needed, so just in case.
  pop	es
  cld
#endif

#if __FIRST_ARG_IN_AX__
  mov	si,ax
  mov	di,[bx+2]
  mov	cx,[bx+4]
#else
  mov	si,[bx+2]	! Fetch
  mov	di,[bx+4]
  mov	cx,[bx+6]
#endif

  inc	cx
lp1:
  dec	cx
  je	lp2
  lodsb
  scasb
  jne	lp3
  testb	al,al
  jne	lp1
lp2:
  xor	ax,ax
  jmp	lp4
lp3:
  sbb	ax,ax
  or	al,#1
lp4:

#ifdef PARANOID
  pop	es
#endif
  pop	di
  pop	si
#endasm
#else
   register char c1=0, c2=0;
   while (l-- >0)
      if ((c1= *d++) != (c2= *s++) || c1 == '\0' )
         break;
   return c1-c2;
#endif
}
