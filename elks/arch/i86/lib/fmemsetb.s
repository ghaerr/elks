// void fmemsetb (word_t off, seg_t seg, byte_t val, word_t count)
// segment after offset to allow LES from the stack
// compiler pushes byte_t as word_t

    .code16

	.text

	.global fmemsetb

fmemsetb:
	mov    %es,%bx
	mov    %di,%dx
	mov    %sp,%di
	mov    6(%di),%ax  // arg2:   value
	mov    8(%di),%cx  // arg3:   byte count
	les    2(%di),%di  // arg0+1: far pointer
	cld
	rep
	stosb
	mov    %dx,%di
	mov    %bx,%es
	ret
