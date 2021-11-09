####################################################################################
# Unreal mode and A20 line management routines
# derived from UNREAL.ASM and A20.ASM code by Chris Giese
# 8 Nov 2021 Greg Haerr
#
# int enable_unreal_mode(void)	- Attempt to turn on 80386 unreal mode,
#				  returns > 0 on success else error code
# int enable_a20_gate(void)	- Enable A20 gate, return 0 on fail
# int verify_a20		- Verify A20 gate status, return 0 if disabled
# void set_a20			- ASM routine: enable (AH=1) or disable (AH=0) A20 gate
# void linear_32fmemcpyw (void *dst_off, addr_t dst_seg, void *src_off, addr_t src_seg,
#		size_t count)	- Copy between XMS and far memory
#
	.code16
	.text

	.global	enable_unreal_mode
	.global	enable_a20_gate
	.global	verify_a20
	.global	set_a20
	.global	linear32_fmemcpyw
	.global	linear32_fmemcpyb

# Try to set unreal mode. Currently requires 386 CPU or better.
# Returns 0 if OK, otherwise error code (1=not 386, 2=in V86 mode).
enable_unreal_mode:
# check for 32-bit CPU
	pushf
		pushf
		popw %bx        # old FLAGS -> BX
		movw %bx,%ax
		xorb $0x70,%ah  # try changing b14 (NT) or b13:b12 (IOPL)
		pushw %ax
		popf
		pushf
		popw %ax        # new FLAGS -> AX
	popf
	xorb %ah,%bh
	xorw %ax,%ax
	andb $0x70,%bh          # 32-bit CPU if we changed NT or IOPL
	je	not_32bit

# check if (32-bit) CPU is in V86 mode
	smsww %bx               # 'SMSW' is a '286+ instruction
	andb $1,%bl
	jne	in_vm86mode

# point gdt_ptr to gdt
	xorl %eax,%eax
	movw %ds,%ax
	shll $4,%eax

	addl $gdt, %eax
	movl %eax, gdt_ptr+2

	cli                     # interrupts off
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
		lgdt gdt_ptr
		movl %cr0, %eax # CR0.PE=1: enable protected mode
		orb $1,%al
		movl %eax, %cr0

		movw $LINEAR_SEL, %bx # selector to segment with 4GB-1 limit
		movw %bx,%ds    # set segment limits in descriptor caches
		movw %bx,%es
		movw %bx,%fs
		movw %bx,%gs

		decb %al        # CR0.PE=0: back to (un)real mode
		movl %eax, %cr0

# loading segment registers in (un)real mode changes their base address
# but not the segment limit; which remains 4GB-1
	popl %gs
	popl %fs
	popl %es
	popl %ds
	//sti

	mov	$1,%ax		# system is in unreal mode, return 1
	ret

not_32bit:
	mov	$-1,%ax		# requires 32 bit CPU
	ret
in_vm86mode:
	mov	$-2,%ax		# CPU in V86 mode
	ret

# Attempt to enable A20 address gate, return 0 on fail
enable_a20_gate:
	mov	$1,%ah		# enable A20
	call	set_a20
	call	verify_a20	# returns 1 if enabled, 0 if disabled
	ret

# verify if A20 gate is enabled, return 0 if disabled
verify_a20:
	push	%ds
	push	%es

	xor	%ax,%ax
	mov	%ax,%ds
	dec	%ax
	mov	%ax,%es

	mov	%es:0x10,%ax	# read word at FFFF:0010 (1 meg)
	not	%ax		# 1's complement
	pushw	0		# save word at 0000:0000 (0)

	mov	%ax,0		# word at 0 = ~(word at 1 meg)
	mov	0,%ax		# read it back
	cmp	%es:0x10,%ax	# fail if word at 0 == word at 1 meg

	popw	0

	pop	%es
	pop	%ds
	jz	1f		# if ZF=1, the A20 gate is NOT enabled
	mov	$1,%ax		# return 1 if enabled
	ret
1:	mov	$0,%ax		# return 0 if disabled
	ret

# enable/disable A20 gate using keyboard port/controller, entry AH=0 disable
set_a20:
	cli
	call	empty_8042
	mov	$0xD0,%al	# 8042 command byte to read output port
	out	%al,$0x64
1:	in	$0x64,%al
	test	$1,%al		# output buffer (data _from_ keyboard) full?
	jz	1b		# no, loop

	in	$0x60,%al	# read output port
	or	%ah,%ah
	jne	2f
	and	$0xFD,%al	# AND ~2 to disable
	jmp	3f
2:	or	$2,%al		# OR 2 to enable
3:	mov	%al,%ah

	call	empty_8042
	mov	$0xD1,%al	# 8042 command byte to write output port
	out	%al,$0x64

	call	empty_8042
	mov	%ah,%al		# the value to write
	out	%al,$0x60

	call	empty_8042
	//sti
	ret

0:
	jmp	1f		# a delay (probably not effective nor necessary)
1:	in	$0x60,%al	# read and discard data/status from 8042
empty_8042:
	jmp	2f		# delay
2:	in	$0x64,%al
	test	$1,%al		# output buffer (data _from_ keyboard) full?
	jnz	0b		# yes, read and discard
	test	$2,%al		# input buffer (data _to_ keyboard) empty?
	jnz	empty_8042	# no, loop
	ret

#
# void linear_32fmemcpyw(void *dst_off, addr_t dst_seg, void *src_off, addr_t src_seg,
#		size_t count)
# WARNING: Requires 32-bit CPU, with unreal mode and A20 gate enabled!
#
linear32_fmemcpyw:
	push   %es
	mov    %si,%ax
	mov    %di,%dx
	xorl   %esi,%esi
	mov    %sp,%si

	xorl   %ecx,%ecx
	mov    16(%si),%cx    // word count -> ECX

	xorl   %ebx,%ebx
	mov    4(%si),%bx     // word dest offset -> EBX
	movl   6(%esi),%edi   // long dest address -> EDI
	addl   %ebx,%edi      // EDI is linear destination

	xorl   %ebx,%ebx
	mov    10(%si),%bx    // word src offset -> EBX
	movl   12(%esi),%esi  // long src address -> ESI
	addl   %ebx,%esi      // ESI is linear source

	xor    %bx,%bx        // ES = DS = 0 for 32-bit linear addressing
	mov    %bx,%es
	mov    %bx,%ds

	cld
	addr32                // 32-bit address prefix
	rep
	movsw                 // word [ES:EDI++] <- [DS:ESI++], ECX times

	mov    %ax,%si
	mov    %dx,%di
	mov    %ss,%ax
	mov    %ax,%ds
	pop    %es
	ret

#
# void linear_32fmemcpyb(void *dst_off, addr_t dst_seg, void *src_off, addr_t src_seg,
#		size_t count)
# WARNING: Requires 32-bit CPU, with unreal mode and A20 gate enabled!
#
linear32_fmemcpyb:
	push   %es
	mov    %si,%ax
	mov    %di,%dx
	xorl   %esi,%esi
	mov    %sp,%si

	xorl   %ecx,%ecx
	mov    16(%si),%cx    // word count -> ECX

	xorl   %ebx,%ebx
	mov    4(%si),%bx     // word dest offset -> EBX
	movl   6(%esi),%edi   // long dest address -> EDI
	addl   %ebx,%edi      // EDI is linear destination

	xorl   %ebx,%ebx
	mov    10(%si),%bx    // word src offset -> EBX
	movl   12(%esi),%esi  // long src address -> ESI
	addl   %ebx,%esi      // ESI is linear source

	xor    %bx,%bx        // ES = DS = 0 for 32-bit linear addressing
	mov    %bx,%es
	mov    %bx,%ds

	cld
	shrl   $1,%ecx        // copy words
	addr32                // 32-bit address prefix
	rep
	movsw                 // word [ES:EDI++] <- [DS:ESI++], ECX times

	rcll    $1,%ecx       // then possibly final byte
	addr32                // 32-bit address prefix
	rep
	movsb                 // byte [ES:EDI++] <- [DS:ESI++], ECX times

	mov    %ax,%si
	mov    %dx,%di
	mov    %ss,%ax
	mov    %ax,%ds
	pop    %es
	ret

# The GDT contains 8-byte DESCRIPTORS for each protected-mode segment.
# Each descriptor contains a 32-bit segment base address, a 20-bit segment
# limit, and 12 bits describing the segment type. The descriptors look
# like this:
#
#           MSB    bit 6   bit 5   bit 4   bit 3   bit 2   bit 1   LSB
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#byte 0  | bit 7<---------------- segment limit------------------->bit 0 |
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#byte 1  |bit 15<---------------- segment limit------------------->bit 8 |
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#byte 2  | bit 7<---------------- segment base-------------------->bit 0 |
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#byte 3  |bit 15<---------------- segment base-------------------->bit 8 |
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#byte 4  |bit 23<---------------- segment base-------------------->bit 16|
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#byte 5  |   P   |      DPL      | <----------- segment type ----------> |
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#
# P is the Segment Present bit. It should always be 1.
#
# DPL is the DESCRIPTOR PRIVILEGE LEVEL. For simple code like this, these
# two bits should always be zeroes.
#
# Segment Type (again, for simple code like this) is hex 12 for data
# segments, hex 1A for code segments.
#
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#byte 6  |   G   |   B   |   0   | avail | bit 19<-- seg limit--->bit 16 |
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#
# G is the Limit Granularity. If zero, the segment limit is in bytes
# (0 to 1M, in 1-byte increments). If one, the segment limit is in 4K PAGES
# (0 to 4G, in 4K increments). For simple code, set this bit to 1, and
# set the segment limit to its highest value (FFFFF hex). You now have
# segments that are 4G in size! The Intel CPUs can address no more than
# 4G of memory, so this is like having no segments at all. No wonder
# protected mode is popular.
#
# B is the Big bit; also called the D (Default) bit. For code segments,
# all instructions will use 32-bit operands and addresses by default
# (BITS 32, in NASM syntax, USE32 in Microsoft syntax) if this bit is set.
# 16-bit protected mode is not very interesting, so set this bit to 1.
#
# None of these notes apply to the NULL descriptor. All of its bytes
# should be set to zero.
#
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#byte 7  |bit 31<---------------- segment base------------------->bit 24 |
#        +-------+-------+-------+-------+-------+-------+-------+-------+

	.data
# Global Descriptor Table
# NULL descriptor (required):
gdt:    .word 0         # limit 15:0
        .word 0         # base 15:0
        .byte 0         # base 23:16
        .byte 0         # access byte (descriptor type)
        .byte 0         # b7-4=flags, b3-0=limit 19:16
        .byte 0         # base 31:24
        
# linear data segment descriptor:
LINEAR_SEL = . - gdt
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x92      # present, ring 0, data, expand-up, writable
# can put zero byte here (instead of 0xCF) to DISABLE unreal mode:
        .byte 0xCF      # page-granular, 32-bit, limit=4GB-1
        .byte 0
gdt_len = . - gdt

gdt_ptr:.word gdt_len - 1
        .long gdt
