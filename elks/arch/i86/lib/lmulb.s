! lmulb.s
! lmulb, lmulub don't preserve cx, dx

	.globl	lmulb
	.globl	lmulub
	.text
	.even

lmulb:

lmulub:
	mul	2(di)
	xchg	ax,bx
	mov	cx,ax
	mul	(di)
	add	bx,ax
	mov	ax,2(di)
	mul	cx
	add	bx,dx
	xchg	ax,bx
	ret
