// byte_t peekb (word_t off, seg_t seg)
// segment after offset to allow LDS from the stack
// returns the byte at the far pointer segment:offset

    .code16

	.text

	.global peekb

peekb:
	mov    %ds,%dx
	mov    %sp,%bx
	lds    2(%bx),%bx  // arg0+1: far pointer
	mov    (%bx),%al  // DS by default
	xor    %ah,%ah
	mov    %dx,%ds
	ret
