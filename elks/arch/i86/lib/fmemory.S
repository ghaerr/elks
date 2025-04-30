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

	.global loadall_block_move

// void fmemcpyb (void * dst_off, seg_t dst_seg, void * src_off, seg_t src_seg,
//		size_t count)

fmemcpyb:
	mov    %si,%ax
	mov    %di,%dx
	mov    %sp,%si
	mov    %es,%bx
	mov    ARG4(%si),%cx  // byte count
	les    ARG0(%si),%di  // far destination pointer
	lds    ARG2(%si),%si  // far source pointer
	cld
	shr    $1,%cx         // copy words
	rep
	movsw
	rcl    $1,%cx         // then possibly final byte
	rep
	movsb
	mov    %bx,%es
	mov    %ax,%si
	mov    %dx,%di
	mov    %ss,%ax
	mov    %ax,%ds
	ret

// void fmemcpyw (void * dst_off, seg_t dst_seg, void * src_off, seg_t src_seg,
//		size_t count)

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

// void fmemsetb (void * off, seg_t seg, byte_t val, size_t count)
// compiler pushes byte_t as word_t

fmemsetb:
	mov    %di,%dx
	mov    %sp,%di
	mov    %es,%bx
	mov    ARG2(%di),%ax  // value
	mov    ARG3(%di),%cx  // byte count
	les    ARG0(%di),%di  // far pointer
	cld
	shr    $1,%cx         // store words
	mov    %al,%ah
	rep
	stosw
	rcl    $1,%cx         // then possibly final byte
	rep
	stosb
	mov    %bx,%es
	mov    %dx,%di
	ret

// void fmemsetw (void * off, seg_t seg, word_t val, size_t count)

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

// int fmemcmpb (void * dst_off, seg_t dst_seg, void * src_off, seg_t src_seg,
//		size_t count)

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

// int fmemcmpw (void * dst_off, seg_t dst_seg, void * src_off, seg_t src_seg,
//		size_t count)

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

loadall_block_move:
# DS:BX gdtp
# CX    word count
# SI    0000h
# DI    0000h
	push	%es
	push	%si
	push	%di
	push	%bp
	mov	%sp,%bp

	xor	%si,%si
	xor	%di,%di

	mov	$0x80,%cx
	mov	%cx,%es	#ES=0080h segment
	mov	$0x33,%cx	#loadall 800-866h
	xor	%ax,%ax
	cli
rep	stosw			#800h-866h clear

	xor	%di,%di
	mov	10(%bp),%bx	# gdtp -> DS:BX

#src DS
	mov	16(%bx),%ax	#src limit
	mov	%ax,%es:(0x4c)	#new DS limit
	mov	18(%bx),%ax	#src addr low16
	mov	%ax,%es:(0x48)	#new DS low16
	mov	20(%bx),%ax	#src addr high8 & access byte
	mov	%ax,%es:(0x4a)	#new DS high8 & access byte
	
#dst ES
	mov	24(%bx),%ax	#dst limit
	mov	%ax,%es:(0x3a)	#new ES limit
	mov	26(%bx),%ax	#dst addr low16
	mov	%ax,%es:(0x36)	#new ES low16
	mov	28(%bx),%ax	#dst addr high8 & access byte
	mov	%ax,%es:(0x38)	#new ES high8 & access byte

#cs
	mov	%cs,%ax
	mov	$12,%cl
	shr	%cl,%ax
	mov	%al,%es:(0x3e)	#CS high8
	mov	%cs,%ax
	mov	$4,%cl
	shl	%cl,%ax
	mov	%ax,%es:(0x3c)	#CS low16
	mov	$-1,%ax
	mov	%ax,%es:(0x40)	#cs limit
	mov	%ax,%es:(0x46)	#ss limit
	mov	%al,%es:(0x5f)	#IDT limit(FF00h but 3FFh?) very important


	mov	$0x9a93,%ax
	mov	%ah,%es:(0x3f)	#cs access byte
	mov	%al,%es:(0x45)	#ss access byte

#ss
	mov	%ss,%ax
	mov	$12,%cl
	shr	%cl,%ax
	mov	%al,%es:(0x44)	#SS high8
	mov	%ss,%ax
	mov	$4,%cl
	shl	%cl,%ax
	mov	%ax,%es:(0x42)	#SS low16

	mov	12(%bp),%cx	# word count

	push	%ss
	push	%cs

	push	%cs
	call	modechange2
	mov	%sp,%bp
	mov	-6(%bp),%ax
	add	$2,%ax
	mov	%ax,%es:(0x1a)	#new IP

	mov	%ds,%es:(0x1e)	#not realaddress
	mov	%ss,%es:(0x20)	#
	mov	%cs,%es:(0x22)	#
	mov	%es,%es:(0x24)	#set 20bit segment
	mov	%di,%es:(0x26)
	mov	%si,%es:(0x28)
	mov	%bp,%es:(0x2a)
	mov	%sp,%es:(0x2c)
	mov	%bx,%es:(0x2e)
	mov	%dx,%es:(0x30)
	mov	%cx,%es:(0x32)
	mov	%ax,%es:(0x34)
.word 0x050f				#loadall for direct binary

modechange2:
	call	retaddr	#3byte
#this address is stacked [bp-6]
	retf			#1byte
retaddr:
	ret			#1byte
#new IP
rep	movsw				#word transfer
	call	modechange2		#retf "cs back to 20bit segment" this is for 32bit CPU noneed?
	pop	%ax
	mov	%ax,%ss		#SS 20bit segment
	push	%ds
	pop	%ds			#used 24bit DS back to 20bit segment
	push	%es
	pop	%es			#used 24bit ES back to 20bit segment
	sti

	pop	%bp
	pop	%di
	pop	%si
	pop	%es
	xor	%ax,%ax		#all success
	ret
