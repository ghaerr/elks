! ldecb.s

	.globl	ldecb
	.globl	ldecub
	.text
	.even

ldecb:

ldecub:
	sub	word ptr 2[bx],#1
	sbb	word ptr [bx],#0
	ret
