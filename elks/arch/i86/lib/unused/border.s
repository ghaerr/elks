// border.s

    .code16

	.global ntohl
	.global htonl
	.text
	.align 1

htonl:
ntohl:
	pop	%bx
	pop	%dx
	pop	%ax
	sub	$4,%sp
	xchg	%al,%ah
	xchg	%dl,%dh
	jmp	*%bx
