| void outw( int value, int port );
| writes the word  value  to  the i/o port  port

	.globl	_outw
	.text
	.even
_outw:
	pop	bx
	pop	ax
	pop	dx
	sub	sp,*4
	outw
	jmp	bx

| void outw_p( int value, int port );
| writes the word  value  to  the i/o port  port

	.globl	_outw_p
	.text
	.even
_outw_p:
	pop	bx
	inb	0x80
	pop	ax
	pop	dx
	sub	sp,*4
	outw
	jmp	bx
