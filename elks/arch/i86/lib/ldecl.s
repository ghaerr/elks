! ldecl.s

	.globl	ldecl
	.globl	ldecul
	.text
	.even

ldecl:

ldecul:
	cmp	word ptr [bx],*0
	je	LDEC_BOTH
	dec	word ptr [bx]
	ret

	.even

LDEC_BOTH:
	dec	word ptr [bx]
	dec	word ptr 2[bx]
	ret
