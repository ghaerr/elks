! int setupw( int *offset );
! returns the word at the far pointer  0x9000:offset

#include <linuxmt/config.h>

	.define	_setupw
	.text
	.even

_setupw:
	mov	cx,ds
	pop	dx

#ifndef CONFIG_ROMCODE
  INITSEG = DEF_INITSEG
#else
  INITSEG = CONFIG_ROM_SETUP_DATA
#endif

#ifndef CONFIG_286PMODE
	mov	ax,#INITSEG
#else
	mov	ax,#0x0048
#endif

	mov	ds,ax
	pop	bx
	sub	sp,*2
	mov	ax,[bx]
	mov	ds,cx
	jmp	dx
