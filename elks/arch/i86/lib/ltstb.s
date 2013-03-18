! ltstb.s
! ltstb, ltstub don't preserve ax

	.globl	ltstb
	.globl	ltstub
	.text
	.even

ltstb:

ltstub:
	test	ax,ax
	jne	LTSTB_RET
	test	bx,bx
	jns	LTST_RET
	inc	ax		! clear ov and mi, set ne for greater than
LTSTB_RET:
	ret
