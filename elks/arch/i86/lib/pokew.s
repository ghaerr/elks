// void pokew (word_t off, seg_t seg, word_t val)
// segment after offset to allow LDS from the stack
// writes the word at the far pointer segment:offset

    .code16

	.text

	.global pokew

pokew:
	mov    %ds,%dx
	mov    %sp,%bx
	mov    6(%bx),%ax  // arg2: value
	lds    2(%bx),%bx  // arg0+1: far pointer
	mov    %ax,(%bx)    // DS by default
	mov    %dx,%ds
	ret
