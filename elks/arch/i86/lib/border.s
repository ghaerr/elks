! border.s

	.globl _ntohl
	.globl _htonl
	.text
	.even

_htonl:
_ntohl:
	pop	bx
	pop	dx
	pop	ax
	sub	sp,#4
	xchg	ah,al
	xchg	dh,dl
	jmp	bx

