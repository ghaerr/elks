! lincl.s

	.globl	lincl
	.globl	lincul
	.text
	.even

lincl:

lincul:
	inc	word ptr [bx]
	jnz	LINC_RET
	inc	word ptr 2[bx]

LINC_RET:
	ret
