! border.s

	.globl _ntohl
	.globl _htonl
	.text
	.even

_htonl:
_ntohl:
	push	bp
	mov	bp,sp
	mov     bx,4[bp]
	mov     ax,6[bp]
	mov	cl,#8
	ror	ax,cl
	ror	bx,cl
	mov	dx,bx
	pop	bp
	ret

