| int inw( int port );
| reads a word from the i/o port  port  and returns it

	.globl	_inw
	.text
	.even
_inw:
	pop	bx
	pop	dx
	dec	sp
	dec	sp
	inw
	jmp	bx

| int inw_p( int port );
| reads a word from the i/o port  port  and returns it

	.globl	_inw_p
	.text
	.even
_inw_p:
	pop	bx
	pop	dx
	dec	sp
	dec	sp
	inb	0x80
	inw
	jmp	bx
