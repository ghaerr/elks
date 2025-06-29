#include <linuxmt/config.h>

//      Assembler boot strap hooks. This is called by setup

        .arch i8086, nojumps
        .code16
        .text

        .global _start
        .extern start_kernel
        .global early_putchar

        .word   0,0,0,0,0,0,0,0 // 16-byte offset for HMA segment FFFF:0000 not in XMS
_start:

/*
! Setup.S already initialized DS and ES (but not SS)
! In addition, registers contain:
!   BX, Text size
!   DI  Far text size
!   SI, Data size
!   DX, BSS size
*/
        mov     %bx,_endtext
        mov     %di,_endftext
        mov     %si,_enddata
        add     %dx,%si
        mov     %si,_endbss

// Start cleaning BSS. Still using setup.S stack

        mov     _enddata,%di    // start of BSS
        mov     %dx,%cx         // CX = BSS size
        xor     %ax,%ax
        shr     $1,%cx
        cld
        rep
        stosw

// End cleaning BSS

        mov     %cs,kernel_cs
        mov     %ds,kernel_ds

// Set SS:SP to kernel interrupt stack for temporary use

        mov     %ds,%ax
        mov     %ax,%ss         // SS=ES=DS
        mov     $tstack,%sp     // can't use kernel interrupt stack, must have temp stack

        call    start_kernel    // no return

        .global int3
int3:   int     $3              // C breakpoint for emu86
        ret

#ifdef CONFIG_ARCH_SWAN
early_putchar:
1:	in	$0xB3,%al
	test	$0x04,%al
	jz	1b
        mov	%sp,%bx
        mov	2(%bx),%al
	out	%al,$0xB1
        ret
#elif defined(CONFIG_ARCH_SOLO86)
early_putchar:
        mov   %sp,%bx
        mov   2(%bx),%al
        outb  %al,$0x22
        ret
#else
#ifndef CONFIG_ARCH_PC98
early_putchar:
        mov   %sp,%bx
        mov   2(%bx),%al
        mov   $0x0E,%ah
        mov   $0x0007,%bx
        push  %bp               // some BIOS may destroy BP
        int   $0x10
        pop   %bp
        ret
#endif
#endif

//      Segment beginnings

// Zero for NULL pointers (near and far)
// Will be linked as first section in data segment
        .section .nildata
        .word   0
        .word   0

        .data
        .global _endtext
        .global _endftext
        .global _enddata
        .global _endbss
        .extern kernel_cs
        .extern kernel_ds
        .extern tstack

_endtext:
        .word   0

_endftext:
        .word   0

_enddata:
        .word   0

_endbss:
        .word   0

        .bss
        .p2align 1
_sbss:
