! void outb( char value, int port);
! writes the byte  value  to  the i/o port  port

	.globl	_outb
	.text
	.even

_outb:
	pop	bx
	pop	ax
	pop	dx
	sub	sp,*4
	out	dx
	jmp	bx

! void outb_p( char value, int port);
! writes the byte  value  to  the i/o port  port

	.globl	_outb_p
	.text
	.even

_outb_p:
	pop	bx
	inb	0x80
	pop	ax
	pop	dx
	sub	sp,*4
	out	dx
	jmp	bx
