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
##############################################################################

    .code16

	.text

	.global	entry
entry:

# set text video mode 80x25 and clear screen
        movw $0x0003,%ax
        int $0x10

# check for 32-bit CPU
        movw	$_cpu16_msg_len,%dx
        movw	$_cpu16_msg,%cx

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
        movw	$_v86_msg_len,%dx
        movw	$_v86_msg,%cx

        smsww %bx               # 'SMSW' is a '286+ instruction
        andb $1,%bl
        jne msg_and_exit

# point _gdt_ptr to _gdt
        xorl %eax,%eax
        movw %ds,%ax
        shll $4,%eax

        addl $_gdt, %eax
        movl %eax, (_gdt_ptr+2)

        cli                     # interrupts off

        pushl %ds
        pushl %es
        pushl %fs
        pushl %gs
                lgdt (_gdt_ptr)
                movl %cr0, %eax # CR0.PE=1: enable protected mode
                orb $1,%al
                movl %eax, %cr0

                movw $DATA_SEL, %bx # selector to segment with 4GB-1 limit
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

# demo use of 32-bit address by copying stuff to bottom line of 80x25 screen
        pushl %es
        xorw %di,%di
        movw %di,%es
        movl $(0xB8000+80*2*4),%edi
        movl $_unreal_msg, %esi
        movl $_unreal_msg_len, %ecx
# Action of "rep movsb" in (un)real mode:
# With a32 prefix byte (0x67):  byte [ES:EDI++] <- [DS:ESI++], ECX times
# Without prefix byte:          byte [ES:DI++] <- [DS:SI++], CX times
        .byte 0x67 #a32
        rep movsb
# or poke byte
        addr32 movb $'!',  %es:0xb8296
        addr32 movb $0x2C, %es:0xb8297
        popl %es

        movw	$_success_msg_len,%dx
        movw	$_success_msg,%cx

msg_and_exit:

        movw	$1,%bx
        movw	$4,%ax
        int	$0x80

        movw	$0,%bx
        movw	$1,%ax
        int	$0x80

        .data

# Global Descriptor Table
# NULL descriptor (required):
_gdt:   .word 0         # limit 15:0
        .word 0         # base 15:0
        .byte 0         # base 23:16
        .byte 0         # access byte (descriptor type)
        .byte 0         # b7-4=flags, b3-0=limit 19:16
        .byte 0         # base 31:24

# code segment descriptor:
CODE_SEL = . - _gdt
        .word 0xFFFF
        .word 0
        .byte 0
        .byte 0x92      # present, ring 0, data, expand-up, writable
        .byte 0x8F
        .byte 0

# data segment descriptor:
DATA_SEL = . - _gdt
        .word 0xFFFF
        .word 0
        .byte 0
        .byte 0x92      # present, ring 0, data, expand-up, writable
# can put zero byte here (instead of 0xCF) to DISABLE unreal mode:
        .byte 0xCF      # 32-bit segment, page-granular 20-bit limit=4GB-1
        .byte 0
_gdt_end:

_gdt_ptr:
            .word _gdt_end - _gdt - 1
            .long _gdt

_cpu16_msg:
        .ascii "Sorry, 32-bit CPU required\n"
        _cpu16_msg_len = . - _cpu16_msg
_v86_msg:
        .ascii "Sorry, CPU in Virtual-8086 mode (Windows DOS box or EMM386 loaded)\n"
        _v86_msg_len = . - _v86_msg
_success_msg:
        .ascii "Unreal mode enabled!\nThe black-on-green text on the screen is written with 32bit offsets.\n"
        _success_msg_len = . - _success_msg

# This string is written directly to text-mode video memory.
# The alternating spaces are treated as character attribute bytes.
# 0x20 = black text (color 0) on green background (color 2)
_unreal_msg:
        .ascii "U n r e a l   m o d e "
        _unreal_msg_len = . - _unreal_msg

