! lorl.s

	.globl	lorl
	.globl	lorul
	.text
	.even

lorl:

lorul:
	or	ax,[di]
	or	bx,2[di]
	ret
