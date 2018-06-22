// void fmemsetw (word_t off, seg_t seg, word_t val, word_t count)
// segment after offset to allow LES from the stack

    .code16

	.text

	.global fmemsetw

fmemsetw:
	mov    %es,%bx
	mov    %di,%dx
	mov    %sp,%di
	mov    6(%di),%ax  // arg2:   value
	mov    8(%di),%cx  // arg3:   byte count
	les    2(%di),%di  // arg0+1: far pointer
	cld
	rep
	stosw
	mov    %dx,%di
	mov    %bx,%es
	ret
