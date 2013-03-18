! lincb.s

	.globl	lincb
	.globl	lincub
	.text
	.even

lincb:

lincub:
	inc	2(bx)
	jne	LINC_END
	inc	(bx)
LINC_END:
	ret
