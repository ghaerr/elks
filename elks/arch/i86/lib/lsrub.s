! lsrub.s
! lsrub doesn't preserve cx

	.globl	lsrub
	.text
	.even

lsrub:
	mov	cx,di
	jcxz	LSRU_EXIT
	cmp	cx,*32
	jae	LSRU_ZERO

LSRU_LOOP:
	shr	ax,*1
	rcr	bx,*1
	loop	LSRU_LOOP

LSRU_EXIT:
	ret

	.even

LSRU_ZERO:
	xor	ax,ax
	mov	bx,ax
	ret
