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
# split enable_unreal_mode and enable_a20_gate into seperate kernel file
##############################################################################

sys_exit = 1
sys_write = 4
syscall = 0x80
videoline = 0xb8000+80*2*22

    .arch	i386,nojumps
    .code16
    .text
	.extern	enable_unreal_mode
	.extern	enable_a20_gate
	.extern	verify_a20
	.extern	set_a20

    .global _start
_start:

	mov	$trying_unreal_msg,%si
	call	puts

	call	check_unreal_mode
	cmp	$-1,%ax
	jnz	1f
	mov     $needs_386_msg,%si
	jmp	msg_and_exit
1:	cmp	$-2,%ax
	jnz	2f
	mov     $v86_msg,%si
	jmp	msg_and_exit

# demo use of 32-bit address by copying stuff directly to memory-mapped screen
2:      call	enable_unreal_mode
	push    %es
	cli			# interrupts off - trashes ESI/EDI/ECX
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
	.byte 0x67		# 80386 B1 step chip bug on mixing addresses
	nop

# or poke byte
        addr32 movb $'!',%es:videoline+22
        addr32 movb $0x2C,%es:videoline+23
	sti			# interrupt back on
        pop     %es

# output unreal enabled message
        mov     $unreal_enabled_msg,%si
	call	puts

# test for A20 enabled, enable if not
#	mov	$1,%ah		# test disable A20
#	call	set_a20

	mov	$verify_a20_msg,%si
	call	puts

	mov	$a20_enabled_msg,%si
	call	verify_a20	# check initial A20 gate status
	cmp	$0,%ax		# 0=disabled
	jnz	2f		# NZ=enabled
	mov	$a20_disabled_msg,%si
2:	call	puts

	mov	$trying_a20_msg,%si
	call	puts

	call	enable_a20_gate	# enable A20 and return gate status
	mov	$a20_enabled_msg,%si
	cmp	$0,%ax		# 0=fail
	jnz	1f		# NZ=enabled
	mov	$a20_disabled_msg,%si
1:	call	puts

# exit - unreal mode and A20 gate should both be enabled
	call	exit

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

trying_unreal_msg:
        .ascii "Trying to enable unreal mode...\n"
        .byte  0

verify_a20_msg:
        .ascii "Verifying A20 gate status...\n"
        .byte  0

trying_a20_msg:
        .ascii "Trying to enable A20 gate...\n"
        .byte  0

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
