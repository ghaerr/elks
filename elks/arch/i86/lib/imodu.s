! imodu.s
! imodu doesn't preserve dx (returns quotient in it)

	.globl imodu
	.text
	.even

imodu:
	xor	dx,dx
	div	bx
	mov	ax,dx		! instruction queue full so xchg slower
	ret
