! isru.s
! isru doesn't preserve cl

	.globl isru
	.text
	.even

isru:
	mov	cl,bl
	shr	ax,cl
	ret
