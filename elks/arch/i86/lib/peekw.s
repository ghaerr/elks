! int peekw( unsigned segment, int *offset );
! returns the word at the far pointer  segment:offset

	.define	_peekw
	.text
	.even

_peekw:
	mov	cx,ds
	pop	dx
	pop	ds
	pop	bx
	sub	sp,*4
	mov	ax,[bx]
	mov	ds,cx
	jmp	dx
