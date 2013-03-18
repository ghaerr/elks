! ltstl.s
! ltstl, ltstul don't preserve bx

	.globl	ltstl
	.globl	ltstul
	.text
	.even

ltstl:
ltstul:
	test	bx,bx
	jne	LTSTL_RET
	test	ax,ax
	jns	LTSTL_RET
	inc	bx		! clear ov and mi, set ne for greater than
LTSTL_RET:
	ret
