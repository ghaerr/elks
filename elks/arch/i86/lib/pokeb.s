! void pokeb( unsigned segment, char *offset, char value );
! writes the byte  value  at the far pointer  segment:offset

	.define	_pokeb
	.text
	.even

_pokeb:
	mov	cx,ds
	pop	dx
	pop	ds
	pop	bx
	pop	ax
	sub	sp,*6
	movb	[bx],al
	mov	ds,cx
	jmp	dx
