# imod.s
# imod doesn't preserve dx (returns quotient in it)

	.globl imod
	.text
	.even

imod:
	cwd
	idiv	bx
# Instruction queue full so xchg slower
	mov	ax,dx
	ret
