| isr.s
| isr doesn't preserve cl

	.globl isr
	.text
	.even

isr:
	mov	cl,bl
	sar	ax,cl
	ret
