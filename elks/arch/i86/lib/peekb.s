| int peekb( unsigned segment, char *offset );
| returns the (unsigned) byte at the far pointer  segment:offset

	.define	_peekb
	.text
	.even
_peekb:
	mov	cx,ds
	pop	dx
	pop	ds
	pop	bx
	sub	sp,*4
	movb	al,[bx]
	subb	ah,ah
	mov	ds,cx
	jmp	dx
