! idiv.s
! idiv_ doesn't preserve dx (returns remainder in it)

	.globl idiv_
	.text
	.even

idiv_:
	cwd
	idiv	bx
	ret
