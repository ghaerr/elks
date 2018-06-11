// word_t peekw (word_t off, seg_t seg)
// segment after offset to allow LDS from the stack
// returns the word at the far pointer segment:offset

    .code16

	.text

	.global peekw

peekw:
	mov    %ds,%dx
	mov    %sp,%bx
	lds    2(%bx),%bx  // arg0+1: far pointer
	mov    (%bx),%ax  // DS by default
	mov    %dx,%ds
	ret
