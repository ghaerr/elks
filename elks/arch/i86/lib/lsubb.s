| lsubb.s

	.globl	lsubb
	.globl	lsubub
	.text
	.even

lsubb:
lsubub:
	sub	bx,2(di)
	sbb	ax,(di)
	ret
