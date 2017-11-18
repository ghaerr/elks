! int setupb(char *offset);
! returns the (unsigned) byte at the far pointer INITSEG:offset

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

	.define	_setupb
	.text
	.even

_setupb:
	mov	cx,ds
	pop	dx
	mov	ax,#INITSEG
	mov	ds,ax
	pop	bx
	sub	sp,*2
	movb	al,[bx]
	subb	ah,ah
	mov	ds,cx
	jmp	dx
