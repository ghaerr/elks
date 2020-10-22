// fmemcpy[bw], fmemset[bw], fmemcmp[bw] routines
//
// segment parameter after offset to allow LDS/LES from the stack
// assume DS=SS, save ES, for GCC-IA16

#define ARG0	2
#define ARG1	4
#define ARG2	6
#define ARG3	8
#define ARG4	10

	.arch	i8086, nojumps
	.code16
	.text

	.global fmemcpyb
	.global fmemcpyw
	.global fmemsetb
	.global fmemsetw
	.global fmemcmpb
	.global fmemcmpw

// void fmemcpyb (byte_t * dst_off, seg_t dst_seg, byte_t * src_off, seg_t src_seg,
//		word_t count)

fmemcpyb:
	mov    %si,%ax
	mov    %di,%dx
	mov    %sp,%si
	push   %es
	mov    ARG4(%si),%cx  // byte count
	les    ARG0(%si),%di  // far destination pointer
	lds    ARG2(%si),%si  // far source pointer
	mov    %cx,%bx
	cld
	shr    $1,%cx         // copy words
	jz     1f
	rep
	movsw
1:	mov    %bx,%cx        // then possibly final byte
	and    $1,%cx
	jz     2f
	movsb
2:	pop    %es
	mov    %ax,%si
	mov    %dx,%di
	mov    %ss,%ax
	mov    %ax,%ds
	ret

// void fmemcpyw (byte * dst_off, seg_t dst_seg, byte * src_off, seg_t src_seg,
//		word_t count)

fmemcpyw:
	mov    %es,%bx
	mov    %si,%ax
	mov    %di,%dx
	mov    %sp,%si
	mov    ARG4(%si),%cx  // word count
	les    ARG0(%si),%di  // far destination pointer
	lds    ARG2(%si),%si  // far source pointer
	cld
	rep
	movsw
	mov    %ax,%si
	mov    %dx,%di
	mov    %ss,%ax
	mov    %ax,%ds
	mov    %bx,%es
	ret

// void fmemsetb (word_t off, seg_t seg, byte_t val, word_t count)
// compiler pushes byte_t as word_t

fmemsetb:
	mov    %di,%dx
	mov    %sp,%di
	push   %es
	mov    ARG2(%di),%ax  // value
	mov    ARG3(%di),%cx  // byte count
	les    ARG0(%di),%di  // far pointer
	mov    %cx,%bx
	cld
	shr    $1,%cx         // store words
	jz     1f
	mov    %al,%ah
	rep
	stosw
1:	mov    %bx,%cx        // then possibly final byte
	and    $1,%cx
	jz     2f
	stosb
2:	pop    %es
	mov    %dx,%di
	ret

// void fmemsetw (word_t off, seg_t seg, word_t val, word_t count)

fmemsetw:
	mov    %es,%bx
	mov    %di,%dx
	mov    %sp,%di
	mov    ARG2(%di),%ax  // value
	mov    ARG3(%di),%cx  // byte count
	les    ARG0(%di),%di  // far pointer
	cld
	rep
	stosw
	mov    %dx,%di
	mov    %bx,%es
	ret

// int fmemcmpb (word_t dst_off, seg_t dst_seg, word_t src_off, seg_t src_seg,
//		word_t count)

fmemcmpb:
	mov    %es,%bx
	mov    %si,%ax
	mov    %di,%dx
	mov    %sp,%si
	mov    ARG4(%si),%cx  // byte count
	les    ARG0(%si),%di  // far destination pointer
	lds    ARG2(%si),%si  // far source pointer
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

// int fmemcmpw (word_t dst_off, seg_t dst_seg, word_t src_off, seg_t src_seg,
//		word_t count)

fmemcmpw:
	mov    %es,%bx
	mov    %si,%ax
	mov    %di,%dx
	mov    %sp,%si
	mov    ARG4(%si),%cx  // byte count
	les    ARG0(%si),%di  // far destination pointer
	lds    ARG2(%si),%si  // far source pointer
	cld
	repz
	cmpsw
	mov    %ax,%si
	mov    %dx,%di
	jz     fmemcmpw_same
	mov    $1,%ax
	jmp    fmemcmpw_exit

fmemcmpw_same:
	xor    %ax,%ax
fmemcmpw_exit:
	mov    %ss,%dx
	mov    %dx,%ds
	mov    %bx,%es
	ret
