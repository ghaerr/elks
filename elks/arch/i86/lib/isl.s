| isl.s
| isl, islu don't preserve cl

	.globl isl
	.globl islu
	.text
	.even

isl:
islu:
	mov	cl,bl
	shl	ax,cl
	ret
