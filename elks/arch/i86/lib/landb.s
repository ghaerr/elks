! landb.s

	.globl	landb
	.globl	landub
	.text
	.even

landb:

landub:
	and	ax,(di)
	and	bx,2(di)
	ret
