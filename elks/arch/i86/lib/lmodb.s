! lmodb.s
! ax:bx % (di):2(di), remainder ax:bx, quotient cx:di, dx not preserved

	.globl	lmodb
	.extern	ldivmod
	.text
	.even

lmodb:
	xchg	ax,bx
	mov	cx,2(di)
	mov	di,(di)
	call	ldivmod		! bx:ax / di:cx, quot di:cx, rem bx:ax
	xchg	ax,bx
	xchg	cx,di
	ret
