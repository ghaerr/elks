! lincb.s

	.globl	lincb
	.globl	lincub
	.text
	.even

lincb:

lincub:
	inc	2(bx)
	je	LINC_HIGH_WORD
	ret

	.even

LINC_HIGH_WORD:
	inc	(bx)
	ret
