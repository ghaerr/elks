| lsubl.s

	.globl	lsubl
	.globl	lsubul
	.text
	.even

lsubl:
lsubul:
	sub	ax,[di]
	sbb	bx,2[di]
	ret
