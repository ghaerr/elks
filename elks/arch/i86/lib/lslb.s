| lslb.s
| lslb, lslub don't preserve cx

	.globl	lslb
	.globl	lslub
	.text
	.even

lslb:
lslub:
	mov	cx,di
	jcxz	LSL_EXIT
	cmp	cx,*32
	jae	LSL_ZERO
LSL_LOOP:
	shl	bx,*1
	rcl	ax,*1
	loop	LSL_LOOP
LSL_EXIT:
	ret

	.even

LSL_ZERO:
	xor	ax,ax
	mov	bx,ax
	ret
