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

early_putchar:
#if defined(CONFIG_ARCH_SWAN)
1:      in      $0xB3,%al
        test    $0x04,%al
        jz      1b
        mov     %sp,%bx
        mov     2(%bx),%al
        out     %al,$0xB1
#elif defined(CONFIG_ARCH_SOLO86)
        mov   %sp,%bx
        mov   2(%bx),%al
        outb  %al,$0x22
#elif defined(CONFIG_ARCH_NECV25)
#include <arch/necv25.h>
        push  %bp
        push  %ds
        mov   %sp, %bp
        mov   6(%bp), %al
        movw  $NEC_HW_SEGMENT, %bx      // load DS to access memmory mapped CPU registers
        movw  %bx, %ds
1:      testb $0x80, %ds:NEC_STIC1      // wait for Tx ready
        jz    1b
        movb  $0x47, %ds:NEC_STIC1      // clear Tx ready flag
        movb  %al, %ds:NEC_TXB1         // send char
        pop   %ds
        pop   %bp
#elif defined(CONFIG_ARCH_IBMPC) || defined(CONFIG_ARCH_8018X)
        mov   %sp,%bx
        mov   2(%bx),%al
        mov   $0x0E,%ah
        mov   $0x0007,%bx
        push  %bp               // some BIOS may destroy BP
        int   $0x10
        pop   %bp
#endif
        ret

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
