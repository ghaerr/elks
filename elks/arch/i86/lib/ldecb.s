! ldecb.s

	.globl	ldecb
	.globl	ldecub
	.text
	.even

ldecb:

ldecub:
	cmp	2(bx),*0
	je	LDEC_BOTH
	dec	2(bx)
	ret

	.even

LDEC_BOTH:
	dec	2(bx)
	dec	(bx)
	ret
