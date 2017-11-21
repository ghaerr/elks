! int setupw( int *offset );
! returns the word at the far pointer SETUP_DATA:offset

#include <linuxmt/config.h>

	.define	_setupw
	.text
	.even

_setupw:
	mov	cx,ds
	pop	dx
	mov	ax,#SETUP_DATA
	mov	ds,ax
	pop	bx
	sub	sp,*2
	mov	ax,[bx]
	mov	ds,cx
	jmp	dx
