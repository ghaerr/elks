// int fmemcmpb (word_t dst_off, seg_t dst_seg, word_t src_off, seg_t src_seg, word_t count)
// segment after offset to allow LDS & LES from the stack
// assume DS=SS (not ES for GCC-IA16)

    .code16

	.text

	.global fmemcmpb

fmemcmpb:
	mov    %es,%bx
	mov    %si,%ax
	mov    %di,%dx
	mov    %sp,%si
	mov    10(%si),%cx  // arg4:   byte count
	les    2(%si),%di   // arg0+1: far destination pointer
	lds    6(%si),%si   // arg2+3: far source pointer
	cld
	repz
	cmpsb
	mov    %ax,%si
	mov    %dx,%di
	jz     fmemcmpb_same
	mov    $1,%ax
	jmp    fmemcmpb_exit

fmemcmpb_same:
	xor    %ax,%ax

fmemcmpb_exit:
	mov    %ss,%dx
	mov    %dx,%ds
	mov    %bx,%es
	ret
