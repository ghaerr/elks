| laddb.s

	.globl	laddb
	.globl	laddub
	.text
	.even

laddb:
laddub:
	add	bx,2(di)
	adc	ax,(di)
	ret
