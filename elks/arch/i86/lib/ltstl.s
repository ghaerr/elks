! ltstl.s
! ltstl, ltstul don't preserve bx

	.globl	ltstl
	.globl	ltstul
	.text
	.even

ltstl:
ltstul:
	test	bx,bx
	je	LTST_NOT_SURE
	ret

	.even

LTST_NOT_SURE:
	test	ax,ax
	js	LTST_FIX_SIGN
	ret

	.even

LTST_FIX_SIGN:
	inc	bx		! clear ov and mi, set ne for greater than
	ret
