##############################################################################
# Demo of "unreal" mode
# Chris Giese   <geezer@execpc.com>     http://my.execpc.com/~geezer
#
# I, the copyright holder of this work, hereby release it into the
# public domain. This applies worldwide. If this is not legally possible:
# I grant any entity the right to use this work for any purpose,
# without any conditions, unless such conditions are required by law.
#
# 27 Feb 2014:
# - Now a full-fledged DOS program; not just a snippet
# - 32-bit addresses in real mode (i.e. "unreal mode") are demonstrated
#   by copying some text directly to video memory using a 32-bit
#   address in EDI and a value of zero in ES.
#
# 31 Oct 2003:
# - Initial release
#
# Unreal mode is identical with real mode with one exception: 32-bit
# addresses greater than 0xFFFF are allowed (they do not cause INT 0x0D,
# as they do in true real mode).
#
# Unreal mode does not work with the CS or SS registers; nor with V86 mode.
#
# 7th Mar 2020:
# Ported to GNU as assembler und ELKS. Added huge unreal mode to support
# code over 64kb. Georg Potthast
#
# 12 Sep 2020:
# Use `_start' rather than `entry' as program entry point. TK Chia
#
# 4 Nov 2021: ghaerr
# bug fixes and cleanup
# add A20 enable and verify (uses A20.ASM code by Chris Giese)
##############################################################################

sys_exit = 1
sys_write = 4
syscall = 0x80
videoline = 0xb8000+80*2*22

    .code16
    .text

    .global _start
_start:

# check for 32-bit CPU
        mov     $needs_386_msg,%si

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
        je msg_and_exit

# check if (32-bit) CPU is in V86 mode
        mov     $v86_msg,%si

        smsww %bx               # 'SMSW' is a '286+ instruction
        andb $1,%bl
        jne msg_and_exit

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
        sti

# demo use of 32-bit address by copying stuff directly to memory-mapped screen
        push    %es
        xor     %di,%di
        mov     %di,%es
        movl    $videoline,%edi
        movl    $unreal_msg,%esi
        movl    $unreal_msg_len,%ecx

# Action of "rep movsb" in (un)real mode:
# With a32 prefix byte (0x67):  byte [ES:EDI++] <- [DS:ESI++], ECX times
# Without prefix byte:          byte [ES:DI++] <- [DS:SI++], CX times
        cld
        .byte 0x67		# addr32
        rep movsb

# or poke byte
        addr32 movb $'!',%es:videoline+22
        addr32 movb $0x2C,%es:videoline+23
        pop     %es

# output unreal enabled message
        mov     $unreal_enabled_msg,%si
	call	puts

# test for A20 enabled, enable if not
#	mov	$1,%ah		# test disable A20
#	call	set_a20

	mov	$a20_enabled_msg,%si
	call	verify_a20
	jnz	1f		# NZ=enabled
	mov	$a20_disabled_msg,%si
	call	puts

	mov	$0,%ah		# enable A20
	call	set_a20

	mov	$a20_enabled_msg,%si
	call	verify_a20
	jnz	1f		# NZ=enabled
	mov	$a20_disabled_msg,%si

1:	call	puts

# exit - unreal mode and A20 gate should both be enabled
	call	exit

# verify if A20 gate is enabled, return Z=disabled, NZ=enabled
verify_a20:
	push	%ax
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
	pop	%ax
	ret			# if ZF=1, the A20 gate is NOT enabled

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
	sti
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

# write string at SI and exit
msg_and_exit:
	call	puts
exit:   mov     $0,%bx
        mov     $sys_exit,%ax
        int     $syscall

# write string at SI
puts:	call	strlen
	mov	$1,%bx
	mov	%si,%cx
	mov	%ax,%dx
	mov     $sys_write,%ax
	int     $syscall
	ret

# return length of string at SI
strlen:	push	%si
	xor	%bx,%bx
	dec	%bx
1:	inc	%bx
	lodsb
	test	%al,%al
	jnz	1b
	mov	%bx,%ax
	pop	%si
        ret

        .data
        
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

needs_386_msg:
        .ascii "Sorry, 80386+ CPU required\n"
        .byte  0
v86_msg:
        .ascii "Sorry, CPU in Virtual-8086 mode\n"
        .byte  0

unreal_enabled_msg:
        .ascii "Unreal mode enabled if line above written with black on green text.\n"
        .byte  0
        
a20_enabled_msg:
	.ascii	"A20 gate enabled\n"
	.byte	0

a20_disabled_msg:
	.ascii	"A20 gate disabled\n"
	.byte	0

# This string is written directly to text-mode video memory.
# The alternating spaces are treated as character attribute bytes.
# 0x20 = black text (color 0) on green background (color 2)
unreal_msg:
        .ascii "U n r e a l   m o d e "
        unreal_msg_len = . - unreal_msg
