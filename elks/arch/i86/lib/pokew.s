| void pokew( unsigned segment, int *offset, int value );
| writes the word value  at the far pointer  segment:offset

	.define	_pokew
	.text
	.even
_pokew:
	mov	cx,ds
	pop	dx
	pop	ds
	pop	bx
	pop	ax
	sub	sp,*6
	mov	[bx],ax
	mov	ds,cx
	jmp	dx
