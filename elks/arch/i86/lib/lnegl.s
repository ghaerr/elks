! lnegl.s

	.globl	lnegl
	.globl	lnegul
	.text
	.even

lnegl:

lnegul:
	neg	bx
	neg	ax
	sbb	bx,*0
	ret
