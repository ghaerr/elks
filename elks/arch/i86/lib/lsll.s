! lsll.s
! lsll, lslul don't preserve cx

	.globl	lsll
	.globl	lslul
	.text
	.even

lsll:

lslul:
	mov	cx,di
	jcxz	LSL_EXIT
	cmp	cx,*32
	jae	LSL_ZERO

LSL_LOOP:
	shl	ax,*1
	rcl	bx,*1
	loop	LSL_LOOP

LSL_EXIT:
	ret

	.even

LSL_ZERO:
	xor	ax,ax
	mov	bx,ax
	ret
