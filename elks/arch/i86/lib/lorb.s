! lorb.s

	.globl	lorb
	.globl	lorub
	.text
	.even

lorb:

lorub:
	or	ax,(di)
	or	bx,2(di)
	ret
