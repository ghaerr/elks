! int inb( int port );
! reads a byte from the i/o port  port  and returns it

	.globl	_inb
	.text
	.even

_inb:
	pop	bx
	pop	dx
	dec	sp
	dec	sp
	in	dx
	sub	ah,ah
	jmp	bx

! int inb( int port );
! reads a byte from the i/o port  port  and returns it. Uses an in
! from port 0x80 to slow the process down.

	.globl	_inb_p
	.text
	.even

_inb_p:
	pop	bx
	pop	dx
	dec	sp
	dec	sp
	in	0x80
	in	dx
	sub	ah,ah
	jmp	bx
