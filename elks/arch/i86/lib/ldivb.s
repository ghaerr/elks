| ldivb.s
| ax:bx / (di):2(di), quotient ax:bx, remainder cx:di, dx not preserved

	.globl	ldivb
	.extern	ldivmod
	.text
	.even

ldivb:
	xchg	ax,bx
	mov	cx,2(di)
	mov	di,(di)
	call	ldivmod		| bx:ax / di:cx, quot di:cx, rem bx:ax
	xchg	ax,di
	xchg	bx,cx
	ret
