#include <string.h>

int
strcmp(const char *d, const char * s)
{
  /* There are a number of ways to do this and it really does depend on the
     types of strings given as to which is better, nevertheless the Glib
     method is quite reasonable so we'll take that */

#ifdef BCC_AX_ASM
#asm
  mov	bx,sp
  push	di
  push	si

#ifdef PARANOID
  push	es
  push	ds	; Im not sure if this is needed, so just in case.
  pop	es
  cld
#endif

#if __FIRST_ARG_IN_AX__
  mov	di,ax		; dest
  mov	si,[bx+2]	; source
#else
  mov	di,[bx+2]	; dest
  mov	si,[bx+4]	; source
#endif
sc_1:
  lodsb
  scasb
  jne	sc_2		; If bytes are diff skip out.
  testb	al,al
  jne	sc_1		; If this byte in str1 is nul the strings are equal
  xor	ax,ax		; so return zero
  jmp	sc_3
sc_2:
  cmc
  sbb	ax,ax		; Collect correct val (-1,1).
  orb	al,#1
sc_3:

#ifdef PARANOID
  pop	es
#endif
  pop	si
  pop	di
#endasm
#else /* ifdef BCC_AX_ASM */
   register char *s1=(char *)d, *s2=(char *)s, c1,c2;
   while ((c1= *s1++) == (c2= *s2++) && c1 );
   return c1 - c2;
#endif /* ifdef BCC_AX_ASM */
}
