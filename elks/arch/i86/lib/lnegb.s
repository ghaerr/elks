! lnegb.s

	.globl	lnegb
	.globl	lnegub
	.text
	.even

lnegb:

lnegub:
	neg	ax
	neg	bx
	sbb	ax,*0
	ret
