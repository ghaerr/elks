| lmull.s
| lmull, lmulul don't preserve cx, dx

	.globl	lmull
	.globl	lmulul
	.text
	.even

lmull:
lmulul:
	mov	cx,ax
	mul	word ptr 2[di]
	xchg	ax,bx
	mul	word ptr [di]
	add	bx,ax
	mov	ax,ptr [di]
	mul	cx
	add	bx,dx
	ret
