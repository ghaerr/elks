| laddl.s

	.globl	laddl
	.globl	laddul
	.text
	.even

laddl:
laddul:
	add	ax,[di]
	adc	bx,2[di]
	ret
