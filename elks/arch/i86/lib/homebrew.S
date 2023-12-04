// functions for communicating with homebrew computers (INT 0x88 API)

#define ARG0	2

	.code16
	.text

	.global	int88

int88:
	mov	%ds,%dx
	mov	%sp,%bx
	mov	ARG0(%bx),%ax
	xchg	%al,%ah		// function number into AH
	int	$0x88
	mov	%dx,%ds
	ret
