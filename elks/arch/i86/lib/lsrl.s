| lsrl.s
| lsrl doesn't preserve cx

	.globl	lsrl
	.text
	.even

lsrl:
	mov	cx,di
	jcxz	LSR_EXIT
	cmp	cx,*32
	jae	LSR_SIGNBIT
LSR_LOOP:
	sar	bx,*1
	rcr	ax,*1
	loop	LSR_LOOP
LSR_EXIT:
	ret

	.even

LSR_SIGNBIT:
	mov	cx,*32		
| equivalent to +infinity in this context
	j	LSR_LOOP
