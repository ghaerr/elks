! lsrb.s
! lsrb doesn't preserve cx

	.globl	lsrb
	.text
	.even

lsrb:
	mov	cx,di
	jcxz	LSR_EXIT
	cmp	cx,*32
	jae	LSR_SIGNBIT

LSR_LOOP:
	sar	ax,*1
	rcr	bx,*1
	loop	LSR_LOOP

LSR_EXIT:
	ret

	.even

LSR_SIGNBIT:
	mov	cx,*32		! equivalent to +infinity in this context
	j	LSR_LOOP
