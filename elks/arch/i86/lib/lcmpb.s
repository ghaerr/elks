| lcmpb.s
| lcmpb, lcmpub don't preserve ax

	.globl	lcmpb
	.globl	lcmpub
	.text
	.even

lcmpb:
lcmpub:
	sub	ax,(di)		| don't need to preserve ax
	je	LCMP_NOT_SURE
	ret

	.even

LCMP_NOT_SURE:
	cmp	bx,2(di)
	jb	LCMP_B_AND_LT	| b (below) becomes lt (less than) as well
	jge	LCMP_EXIT	| ge and already ae
				| else make gt as well as a (above)
	inc	ax		| clear ov and mi, set ne for greater than
LCMP_EXIT:
	ret

	.even

LCMP_B_AND_LT:
	dec	ax		| clear ov, set mi and ne for less than
	ret
