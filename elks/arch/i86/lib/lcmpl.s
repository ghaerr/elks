! lcmpl.s
! lcmpl, lcmpul don't preserve bx

	.globl	lcmpl
	.globl	lcmpul
	.text
	.even

lcmpl:

lcmpul:
	sub	bx,2[di]	! don't need to preserve bx
	je	LCMP_NOT_SURE
	ret

	.even

LCMP_NOT_SURE:
	cmp	ax,[di]
	jb	LCMP_B_AND_LT	! b (below) becomes lt (less than) as well
	jge	LCMP_EXIT	! ge and already ae
				! else make gt as well as a (above)
	inc	bx		! clear ov and mi, set ne for greater than

LCMP_EXIT:
	ret

	.even

LCMP_B_AND_LT:
	dec	bx		! clear ov, set mi and ne for less than
	ret
