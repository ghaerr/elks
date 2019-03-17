// void fmemcpyb (byte_t * dst_off, seg_t dst_seg, byte_t * src_off, seg_t src_seg, word_t count)
// segment after offset to allow LDS & LES from the stack
// assume DS=ES=SS (not ES for GCC-IA16)

    .code16

	.text

	.global fmemcpyb

fmemcpyb:
#ifdef USE_IA16
	mov    %es,%bx
#endif
	mov    %si,%ax
	mov    %di,%dx
	mov    %sp,%si
	mov    10(%si),%cx  // arg4:   word count
	les    2(%si),%di   // arg0+1: far destination pointer
	lds    6(%si),%si   // arg2+3: far source pointer
	cld
	rep
	movsb
	mov    %ax,%si
	mov    %dx,%di
	mov    %ss,%ax
	mov    %ax,%ds
#ifdef USE_IA16
	mov    %bx,%es
#else
	mov    %ax,%es
#endif
	ret
