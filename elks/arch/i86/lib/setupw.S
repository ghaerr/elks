! int setupw( int *offset );
! returns the word at the far pointer INITSEG:offset

#include <linuxmt/config.h>

#if defined(CONFIG_286PMODE)
  INITSEG = 0x0048
#elif defined(REL_SYS)
  INITSEG = REL_INITSEG
#elif defined(CONFIG_ROMCODE)
  INITSEG = CONFIG_ROM_SETUP_DATA
#else
  INITSEG = DEF_INITSEG
#endif

	.define	_setupw
	.text
	.even

_setupw:
	mov	cx,ds
	pop	dx
	mov	ax,#INITSEG
	mov	ds,ax
	pop	bx
	sub	sp,*2
	mov	ax,[bx]
	mov	ds,cx
	jmp	dx
