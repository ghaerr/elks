| imul.s
| imul_, imul_u don't preserve dx

	.globl imul_
	.globl imul_u
	.text
	.even

imul_:
imul_u:
	imul	bx
	ret
