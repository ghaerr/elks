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

// void loadall_block_move(struct gdt_table *gdtp, size_t words)
// This function must only be called using 80286 CPU!!
//
// LOADALL Memory Format (0800h - 0865h)
// Physical     Size in
// Address	    Words   Associated CPU Register
// 800          3       unused
// 806          1       MSW
// 808          7       unused
// 816          1       TR (Task Register)
// 818          1       Flag Word
// 81A          1       IP
// 81C          1       LDT (Local Descriptor Table)
// 81E          1       DS
// 820          1       SS
// 822          1       CS
// 824          1       ES
// 826          1       DI
// 828          1       SI
// 82A          1       BP
// 82C          1       SP
// 82E          1       BX
// 830          1       DX
// 832          1       CX
// 834          1       AX
// 836          3       ES Descriptor Cache
//  	836         1       ES base low  15:0
//  	838         1       ES base high 23:0 & access byte
//  	83A         1       ES limit     15:0
// 83C          3       CS Descriptor Cache
//  	83C         1       CS base low  15:0
//  	83E         1       CS base high 23:0 & access byte
//  	840         1       CS limit     15:0
// 842          3       SS Descriptor Cache
//  	842         1       SS base low  15:0
//  	844         1       SS base high 23:0 & access byte
//  	846         1       SS limit     15:0
// 848          3       DS Descriptor Cache
//  	848         1       DS base low  15:0
//  	84A         1       DS base high 23:0 & access byte
//  	84C         1       DS limit     15:0
// 84E          3       GDTR (Global Descriptor Table Register)
// 854          3       LDT Descriptor Cache
// 85A          3       IDTR (Interrupt Descriptor Table Register)
// 860          3       TSS (Task State Segment Descriptor Cache)
// total =      66h bytes
//
// Access Byte:
// +-------+-------+-------+-------+-------+-------+-------+-------+
// |   P   |      DPL      |   S   |   E   |  D/C  |  W/R  |   A   |
// +-------+-------+-------+-------+-------+-------+-------+-------+
//      P)resent = 1
//      S)ystem  = 0
//      E)xecutable = 1 for code, 0 for data
//      D)irection/C)onforming = 0
//      W)ritable for data, R)eadable for code = 1
//      A)ccessed = 0
// Normal code segment access byte = 9Ah, data segment access byte = 92h
//
// LOADALL loads all CPU registers from memory physical addresses 800-865h.
// The mechanism of action using LOADALL is that the CPU registers are
// all immediately set to the values set in memory without any checking.
// In particular, the DS and ES descriptor cache register base values are
// set to any physical address within the 24-bit physical address space.
// Once set, the CPU uses the cache base (and limit) values directly without
// referencing the segment register. This allows the REP MOVS after LOADALL
// to move data between any memory locations without the normal 20-bit
// addressing limitation in real mode.
// The segment descriptor cache register base value remains as set until
// the associated segment register is reloaded via a MOV or POP (for data segments)
// or FAR JMP/CALL (for code).
// After exit from this routine, all segment registers are returned to their initial
// values, and all segment descriptor cache base values are reset to their respective
// segment 20-bit real mode value. It is currently untested whether the descriptor
// cache limit values and access bytes are reset, but this won't matter since the
// limits are always passed as FFFFh and the access bytes normal in the GDT table
// passed to this function.
//
loadall_block_move:
	push	%es		#FIXME rewrite prologue to avoid saving DS/ES twice
	push	%si
	push	%di
	push	%bp
	mov	%sp,%bp

	mov	$0x80,%cx	#clear 33h words at 800-866h
	mov	%cx,%es
	mov	$0x33,%cx
	xor	%ax,%ax
	xor	%di,%di
	cli                     #no interrupts, xms_fmemcpyw callable at interrupt time
	cld
	rep	stosw

	mov	10(%bp),%bx	#BX=GDT

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

# code segment cache entry
	mov	%cs,%ax
	mov	$12,%cl
	shr	%cl,%ax
	mov	%al,%es:(0x3e)	#CS high8
	mov	%cs,%ax
	mov	$4,%cl
	shl	%cl,%ax
	mov	%ax,%es:(0x3c)	#CS low16
	mov	$0xFFFF,%ax
	mov	%ax,%es:(0x40)	#CS limit
	mov	%ax,%es:(0x46)	#SS limit
	mov	%al,%es:(0x5f)	#IDTR limit(FF00h not 3FFh?) Must not be zero!

	mov	$0x9a92,%ax
	mov	%ah,%es:(0x3f)	#cs access byte 9A = P, S, E, R bits
	mov	%al,%es:(0x45)	#ss access byte 92 = P, S,    W bits

# stack segment cache entry
	mov	%ss,%ax
	mov	$12,%cl
	shr	%cl,%ax
	mov	%al,%es:(0x44)	#SS high8
	mov	%ss,%ax
	mov	$4,%cl
	shl	%cl,%ax
	mov	%ax,%es:(0x42)	#SS low16

	mov	12(%bp),%cx	#word count

	push	%ss		#save SS for later SS reset
	push	%cs		#CS used in later call to resetCS

	movw	$newip,%es:(0x1a) #new IP after LOADALL
	mov	%ds,%es:(0x1e)	#preserve all segment registers though values not used
	mov	%ss,%es:(0x20)
	mov	%cs,%es:(0x22)
	mov	%es,%es:(0x24)
	mov	%bp,%es:(0x2a)  #preserve normal registers
	mov	%sp,%es:(0x2c)
	//mov	%di,%es:(0x26)	#set SI, DI zero for REP MOVS below
	//mov	%si,%es:(0x28)
	//mov	%bx,%es:(0x2e)	#the following not required to be preserved
	//mov	%dx,%es:(0x30)
	//mov	%cx,%es:(0x32)
	//mov	%ax,%es:(0x34)
	.byte   0x0F,0x05       #loadall opcode - execution continues at new IP
	# not reached

resetCS:
	retf			#reset CS to 20-bit segment

newip:				#new IP after LOADALL, interrupts disabled by FLAGS = 0
	rep	movsw		#word transfer using DS&ES descriptor cache base values

	call	resetCS		#RETF resets CS to 20-bits, uses previous CS from stack
	pop	%ax
	mov	%ax,%ss		#reset SS base FIXME use POP %ss after testing
	push	%ds
	pop	%ds		#reset DS base FIXME pop from rewritten prologue
	push	%es
	pop	%es		#reset ES base FIXME pop from rewritten prologue
	sti

	pop	%bp
	pop	%di
	pop	%si
	pop	%es
	ret
