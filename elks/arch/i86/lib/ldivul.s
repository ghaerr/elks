! ldivul.s
! unsigned bx:ax / 2(di):(di), quotient bx:ax,remainder di:cx, dx not preserved

	.globl	ldivul
	.extern	ludivmod
	.text
	.even

ldivul:
	mov	cx,[di]
	mov	di,2[di]
	call	ludivmod	! unsigned bx:ax / di:cx, quot di:cx, rem bx:ax
	xchg	ax,cx
	xchg	bx,di
	ret
