! int setupb(char *offset);
! returns the (unsigned) byte at the far pointer SETUP_DATA:offset

#include <linuxmt/config.h>

	.define	_setupb
	.text
	.even

_setupb:
	mov	cx,ds
	pop	dx
	mov	ax,#SETUP_DATA
	mov	ds,ax
	pop	bx
	sub	sp,*2
	movb	al,[bx]
	subb	ah,ah
	mov	ds,cx
	jmp	dx
