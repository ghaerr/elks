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
# void linear32_fmemcpyw (void *dst_off, addr_t dst_seg, void *src_off, addr_t src_seg,
#		size_t count)	- Copy words between XMS and far memory
# void linear32_fmemcpyb (void *dst_off, addr_t dst_seg, void *src_off, addr_t src_seg,
#		size_t count)	- Copy bytes between XMS and far memory
# NOTE: the linear32_fmemcpy routines use "unprotected" EBX, EBX, ESI and EDI registers,
#  and interrupts are left on so as to minimize device driver problems.
#  This means that these routines CANNOT be called from an interrupt service routine,
#  as the unprotected registers would be overwritten. This could be solved by
#  turning interrupts off in the routine at the cost of delaying interrupt service
#  for the duration of the copy, which might be too long for time-sensitive drivers,
#  like fast serial ports. Further, no other routines should be written that use
#  the 32-bit register set without coordinating with the functions in this file.
#

# configurable options for A20 gate
#   USE_BIOS		- use BIOS AX=2401h INT 15h method (always tried first)
#   USE_A20ASM		- use Chris Giese A20.ASM method (send D0, read, OR 2, D1, write)
#   USE_HIMEM_AT	- use himem.sys PC AT method (send D1,DF,FF)
#define USE_A20ASM
//#define USE_HIMEM_AT

	.arch	i386,nojumps
	.code16
	.text

	.global	check_unreal_mode
	.global	enable_unreal_mode
	.global	enable_a20_gate
	.global	verify_a20
	.global	set_a20
	.global	linear32_fmemcpyw
	.global	linear32_fmemcpyb
	.global	linear32_fmemset

# Check if unreal mode capable. Currently requires 32-bit CPU (386+)
# Returns 1 if OK, otherwise error code (-1=not 386, -2=in V86 mode).
check_unreal_mode:
	pushf			# check for 32-bit CPU
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

	mov	$1,%ax		# unreal mode capable, return 1
	ret
not_32bit:
	mov	$-1,%ax		# requires 32 bit CPU
	ret
in_vm86mode:
	mov	$-2,%ax		# CPU in V86 mode
	ret

# Put CPU in unreal mode. Requires 32-bit CPU (386+) to call!
enable_unreal_mode:
# point gdt_ptr to gdt
	xorl %eax,%eax
	movw %ds,%ax
	shll $4,%eax

	addl $gdt, %eax
	movl %eax, gdt_ptr+2

	pushf			# save interrupt status
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
	popf			# restore interrupt status

	mov	$1,%ax		# system is in unreal mode, return 1
	ret

# Attempt to enable A20 address gate, return 0 on fail
enable_a20_gate:
	mov	$1,%ah		# enable A20
	call	bios_set_a20	# try BIOS first
	call	verify_a20	# returns 1 if enabled, 0 if disabled
	and	%ax,%ax
	jnz	1f
	mov	$1,%ah		# enable A20
	call	set_a20		# call configurable A20 handler
	call	verify_a20	# returns 1 if enabled, 0 if disabled
1:	ret

# verify if A20 gate is enabled, return 0 if disabled
verify_a20:
	push	%ds
	push	%es

	xor	%ax,%ax
	mov	%ax,%ds
	dec	%ax
	mov	%ax,%es

	pushf			# save interrupt status
	cli			# interrupts off

	mov	%es:0x10,%ax	# read word at FFFF:0010 (1 meg)
	not	%ax		# 1's complement
	pushw	0		# save word at 0000:0000 (0)

	mov	%ax,0		# word at 0 = ~(word at 1 meg)
	mov	0,%ax		# read it back
	cmp	%es:0x10,%ax	# fail if word at 0 == word at 1 meg

	popw	0

	jz	1f		# if ZF=1, the A20 gate is NOT enabled
	mov	$1,%ax		# return 1 if enabled
9:	popf			# restore interrupt status
	pop	%es
	pop	%ds
	ret
1:	mov	$0,%ax		# return 0 if disabled
	jmp	9b

#
# enable/disable A20 gate using keyboard port/controller, entry AH=0 disable
bios_set_a20:
	or	%ah,%ah
	jz	bios_reset_a20
	mov	$0x2401,%ax	# BIOS enable A20
	int	$0x15		# CF clear on success
	ret
bios_reset_a20:
	mov	$0x2400,%ax	# BIOS disable A20
	int	$0x15
	ret

#ifdef USE_A20ASM
set_a20:
	pushf			# save interrupt status
	cli			# interrupts off
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
	popf			# restore interrupt status
	ret

0:	jmp	1f		# a delay (probably not effective nor necessary)
1:	jmp	2f
2:	in	$0x60,%al	# read and discard data/status from 8042
empty_8042:
	jmp	1f
1:	jmp	2f		# delay
2:	in	$0x64,%al
	test	$1,%al		# output buffer (data _from_ keyboard) full?
	jnz	0b		# yes, read and discard
	test	$2,%al		# input buffer (data _to_ keyboard) empty?
	jnz	empty_8042	# no, loop
	ret
#endif

#ifdef USE_HIMEM_AT
set_a20:
	pushf			# save interrupt status
	cli			# interrupts off
	xor	%ah,%ah
	jz	reset_a20

	call	sync_8042
	jnz	a20_err

	mov	$0xD1,%al	# send D1h (write output)
	out	%al,$0x64
	call	sync_8042
	jnz	a20_err

	mov	$0xDF,%al	# send DFh
	out	%al,$0x60
	call	sync_8042
	jnz	a20_err

	# wait for A20 line to settle down (up to 20 usecs)
	mov	$0xFF,%al	# send FFh (pulse output port NULL)
	out	%al,$0x64
	call	sync_8042
a20_err:
	popf			# restore interrupt status
	ret
reset_a20:
	call	sync_8042
	jnz	a20_err

	mov	$0xD1,%al	# send D1h (write output)
	out	%al,$0x64
	call	sync_8042
	jnz	a20_err

	mov	$0xDD,%al	# send DDh
	out	%al,$0x60
	call	sync_8042
	jnz	a20_err


	# wait for A20 line to settle down (up to 20 usecs)
	mov	$0xFF,%al	# send FFh (pulse output port NULL)
	out	%al,$0x64
	call	sync_8042
	popf			# restore interrupt status
	ret

sync_8042:
	xor	%cx,%cx
1:	in	$0x64,%al
	and	$2,%al
	loopnz	1b
	ret
#endif

#if UNUSED
#
# word_t linear32_peekw(void *src_off, addr_t src_seg)
# WARNING: Requires 32-bit CPU, with unreal mode and A20 gate enabled!
#          Trashes EBX and ESI without saving.
#
	.global	linear32_peekw
linear32_peekw:
	call   setgpf
	mov    %si,%dx

	//pushf               // save interrupt status
	//cli                 // uncomment if extended registers used in interrupt routines
	xorl   %esi,%esi
	mov    %sp,%si

	xorl   %ebx,%ebx
	mov    2(%si),%bx     // word src offset -> EBX
	movl   4(%esi),%esi   // long src address -> ESI
	addl   %ebx,%esi      // ESI is linear destination

	xor    %bx,%bx        // DS = 0 for 32-bit linear addressing
	mov    %bx,%ds

	cld
	addr32 lodsw          // AX = [DS:ESI++]
	//popf                // restore interrupt status

	mov    %dx,%si
	mov    %ss,%bx
	mov    %bx,%ds
	ret
#endif

#
# void linear32_fmemcpyw(void *dst_off, addr_t dst_seg, void *src_off, addr_t src_seg,
#		size_t count)
# WARNING: Requires 32-bit CPU, with unreal mode and A20 gate enabled!
#          Trashes EBX, ECX, ESI and EDI without saving.
#
linear32_fmemcpyw:
	push   %es
	mov    %si,%ax
	mov    %di,%dx

	//pushf               // save interrupt status
	//cli                 // uncomment if extended registers used in interrupt routines
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
	addr32 rep movsw      // word [ES:EDI++] <- [DS:ESI++], ECX times
	addr32 nop            // 80386 B1 step chip bug on address size mixing

	//popf                // restore interrupt status

	mov    %ax,%si
	mov    %dx,%di
	mov    %ss,%ax
	mov    %ax,%ds
	pop    %es
	ret

#
# void linear32_fmemcpyb(void *dst_off, addr_t dst_seg, void *src_off, addr_t src_seg,
#		size_t count)
# WARNING: Requires 32-bit CPU, with unreal mode and A20 gate enabled!
#          Trashes EBX, ECX, ESI and EDI without saving.
#
linear32_fmemcpyb:
	push   %es
	mov    %si,%ax
	mov    %di,%dx

	//pushf               // save interrupt status
	//cli                 // uncomment if extended registers used in interrupt routines
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
	addr32 rep movsw      // word [ES:EDI++] <- [DS:ESI++], ECX times
	addr32 nop            // 80386 B1 step chip bug on address size mixing

	rcll    $1,%ecx       // then possibly final byte
	addr32 rep movsb      // byte [ES:EDI++] <- [DS:ESI++], ECX times
	addr32 nop            // 80386 B1 step chip bug on address size mixing

	//popf                // restore interrupt status

	mov    %ax,%si
	mov    %dx,%di
	mov    %ss,%ax
	mov    %ax,%ds
	pop    %es
	ret

#
# void linear32_fmemset(void *dst_off, addr_t dst_seg, byte_t val, size_t count)
# WARNING: Requires 32-bit CPU, with unreal mode and A20 gate enabled!
#          Trashes EAX, EBX, ECX, ESI and EDI without saving.
#
linear32_fmemset:
	push   %si
	push   %es
	mov    %di,%dx

	//pushf               // save interrupt status
	//cli                 // uncomment if extended registers used in interrupt routines
	xorl   %esi,%esi
	mov    %sp,%si

	xorl   %ecx,%ecx
	mov    14(%si),%cx    // word count -> ECX

	xorl   %ebx,%ebx
	mov    6(%si),%bx     // word dest offset -> EBX
	movl   8(%esi),%edi   // long dest address -> EDI
	addl   %ebx,%edi      // EDI is linear destination

	xorl   %eax,%eax
	mov    12(%si),%al    // byte val -> EAX
	mov    %al,%ah

	xor    %bx,%bx        // ES = DS = 0 for 32-bit linear addressing
	mov    %bx,%es
	mov    %bx,%ds

	cld
	shrl   $1,%ecx        // store words
	addr32 rep stosw      // word [ES:EDI++] <- AX, ECX times
	addr32 nop            // 80386 B1 step chip bug on address size mixing

	rcll    $1,%ecx       // then possibly final byte
	addr32 rep stosb      // byte [ES:EDI++] <- AL, ECX times
	addr32 nop            // 80386 B1 step chip bug on address size mixing

	//popf                // restore interrupt status

	mov    %dx,%di
	mov    %ss,%ax
	mov    %ax,%ds
	pop    %es
	pop    %si
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
#byte 5  |   P   |      DPL      |   1   |   0   |  E/C  |  W/R  |   A   |
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#
# This is the access/descriptor type byte, which sets the type of descriptor,
# and the permissions for the associated segment. The permissions are
# interpreted differently based on whether the segment is loaded into
# a code, data, or stack segment register.
#
# P is the Segment Present bit. It should always be 1.
#
# DPL is the DESCRIPTOR PRIVILEGE LEVEL. For simple code like this, these
# two bits should always be zeroes.
#
# For data or executable Segment Descriptors, the next bits are always 1,0.
#
# The E/C bit specifies Expand-down for data segments, Conforming for code.
#
# The W/R bit specifies Writable for data segments, Readable for code.
#
# The A bit is written to 1 by the CPU when the segment is accessed.
#
# For unreal mode data, the P, W and A bits would be set, which gives hex 93.
#
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#byte 6  |   G   |  B/D  |   0   | Avail | bit 19<-- seg limit--->bit 16 |
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#
# G is the Limit Granularity. If zero, the segment limit is in bytes
# (0 to 1M, in 1-byte increments). If one, the segment limit is in 4K PAGES
# (0 to 4G, in 4K increments). For unreal mode, set this bit to 1, and
# set the segment limit to its highest value (FFFFF hex). You now have
# segments that are 4G in size! The Intel CPUs can address no more than
# 4G of memory, so this is like having no segments at all. No wonder
# protected mode is popular.
#
# For code segments, B is the Big bit. When set, this specifies that
# all instructions will use 32-bit operands and addresses by default
# (BITS 32, in NASM syntax, USE32 in Microsoft syntax). For 16-bit
# code segments, the operand and address override instruction prefixes
# (hex 66 and 67) are used to specify access to 32-bit data, which are used
# to implement unreal mode access to extended memory.
#
# For data segments, D is the Default bit. When set, this specifies the
# width of ESP/SP when used as a stack segment. It should be set to 1
# when creating a 32-bit sized segment, otherwise 0.
#
# The Avail bit is available for operating system use by the programmer.
#
# For unreal mode data, the G and D bits would be set, along with bits 19:16
# specifying a 4GB-1 segment limit, giving hex CF.
#
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#byte 7  |bit 31<---------------- segment base------------------->bit 24 |
#        +-------+-------+-------+-------+-------+-------+-------+-------+
#
# None of these notes apply to the NULL descriptor. All of its bytes
# should be set to zero.
#

        .data
        .p2align 1
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
        .byte 0x93      # present, ring 0, data, expand-up, writable, accessed
# putting a zero byte here (instead of 0xCF) effectively disables unreal mode:
#       .byte 0         # byte-granular, 16-bit, limit=64K-1
        .byte 0xCF      # page-granular, 32-bit, limit=4GB-1
        .byte 0
gdt_len = . - gdt

gdt_ptr:.word gdt_len - 1
        .long gdt
