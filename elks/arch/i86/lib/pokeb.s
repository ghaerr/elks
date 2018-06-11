// void pokeb (word_t off, seg_t seg, byte__t val)
// segment after offset to allow LDS from the stack
// compiler pushes byte_t as word_t
// writes the byte at the far pointer segment:offset

    .code16

	.text

	.global pokeb

pokeb:
	mov    %ds,%dx
	mov    %sp,%bx
	mov    6(%bx),%ax  // arg2: value
	lds    2(%bx),%bx  // arg0+1: far pointer
	mov    %al,(%bx)    // DS by default
	mov    %dx,%ds
	ret
