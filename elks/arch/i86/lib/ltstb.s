! ltstb.s
! ltstb, ltstub don't preserve ax

	.globl	ltstb
	.globl	ltstub
	.text
	.even

ltstb:

ltstub:
	test	ax,ax
	je	LTST_NOT_SURE
	ret

	.even

LTST_NOT_SURE:
	test	bx,bx
	js	LTST_FIX_SIGN
	ret

	.even

LTST_FIX_SIGN:
	inc	ax		! clear ov and mi, set ne for greater than
	ret
