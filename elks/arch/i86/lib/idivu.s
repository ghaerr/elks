! idivu.s
! idiv_u doesn't preserve dx (returns remainder in it)

	.globl idiv_u
	.text
	.even

idiv_u:
	xor	dx,dx
	div	bx
	ret
