! landl.s

	.globl	landl
	.globl	landul
	.text
	.even

landl:

landul:
	and	ax,[di]
	and	bx,2[di]
	ret
