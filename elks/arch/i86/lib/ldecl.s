! ldecl.s

	.globl	ldecl
	.globl	ldecul
	.text
	.even

ldecl:

ldecul:
	sub	word ptr [bx],#1
	sbb	word ptr 2[bx],#0
	ret
