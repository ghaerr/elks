! ldivub.s
! unsigned ax:bx / (di):2(di), quotient ax:bx,remainder cx:di, dx not preserved

	.globl	ldivub
	.extern	ludivmod
	.text
	.even

ldivub:
	xchg	ax,bx
	mov	cx,2(di)
	mov	di,(di)
	call	ludivmod	! unsigned bx:ax / di:cx, quot di:cx, rem bx:ax
	xchg	ax,di
	xchg	bx,cx
	ret
