! int setupb(char *offset);
! returns the (unsigned) byte at the far pointer  0x9000:offset

#include <linuxmt/config.h>

	.define	_setupb
	.text
	.even

_setupb:
	mov	cx,ds
	pop	dx

#ifndef CONFIG_286PMODE
	mov	ax,#DEF_INITSEG
#else
	mov	ax,#0x0048
#endif

	mov	ds,ax
	pop	bx
	sub	sp,*2
	movb	al,[bx]
	subb	ah,ah
	mov	ds,cx
	jmp	dx
