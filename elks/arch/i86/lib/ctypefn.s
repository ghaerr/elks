	.text
	.even
! char toupper(char c) { return(islower(c) ? (c ^ 0x20) : (c)); }
	.globl	_toupper
_toupper:
	pop	bx
	pop	ax	! c
	cmpb	al, #'a
	jc	.0
	cmpb	al, #'z
	ja	.0
	j	.1

! char tolower(char c) { return(isupper(c) ? (c ^ 0x20) : (c)); }
	.globl	_tolower
_tolower:
	pop	bx
	pop	ax	! c
	cmpb	al, #'A
	jc	.0
	cmpb	al, #'Z
	ja	.0
.1:
	xor	al,*$20
.0:
	xor	ah, ah
	dec	sp
	dec     sp
	jmp	bx
