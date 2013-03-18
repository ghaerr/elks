! lsrb.s
! lsrb doesn't preserve cx

	.globl	lsrb
	.text
	.even

lsrb:
	mov	cx,di
	jcxz	LSR_EXIT
	cmp	cx,*32
	jb	LSR_LOOP
	mov	cx,*32		! equivalent to +infinity in this context

LSR_LOOP:
	sar	ax,*1
	rcr	bx,*1
	loop	LSR_LOOP

LSR_EXIT:
	ret

