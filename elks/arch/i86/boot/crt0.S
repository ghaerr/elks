// enable the autoconfig tool

#define __ASSEMBLY__

#include <linuxmt/config.h>
#include <arch/asm-offsets.h>

//	Assembler boot strap hooks. This is called by setup

	.arch i8086, nojumps
	.code16
	.text

	.global _start
	.extern	start_kernel
	.global early_putchar

_start:

/*
! Setup.S already initialized DS and ES (but not SS)
! In addition, registers contain:
!   BX, Text size
!   DI  Far text size
!   SI, Data size
!   DX, BSS size
*/
	mov	%bx,_endtext
	mov	%di,_endftext
	mov	%si,_enddata
	add	%dx,%si
	mov	%si,_endbss

// Start cleaning BSS. Still using setup.S stack

	mov	_enddata,%di	// start of BSS
	mov	%dx,%cx		// CX = BSS size
	xor	%ax,%ax
	shr	$1,%cx
	cld
	rep
	stosw

// End cleaning BSS

	mov	%cs,kernel_cs
	mov	%ds,kernel_ds

// Set SS:SP to task[0] kernel stack area

	mov	%ds,%ax
	mov	%ax,%ss		// SS=ES=DS
	mov	$task + TASK_USER_AX,%sp

	call	start_kernel	// fall through into breakpoint if returns

	.global int3
int3:	int	$3		// C breakpoint for emu86
	ret

#ifdef CONFIG_ARCH_PC98
early_putchar:
	mov %sp,%bx
	mov 2(%bx),%al
	push %ds
	push %bx
	push %di
	push %es
	push %dx
	xor %ah,%ah
	push %cs
	pop %ds
	mov $tvram_x,%bx
	mov (%bx),%di
	mov $0xa000,%dx
	mov %dx,%es
	mov %ax,%es:(%di)
	inc %di
	inc %di
	mov %di,(%bx)
	pop %dx
	pop %es
	pop %di
	pop %bx
	pop %ds
	ret
tvram_x:
    .word 800
#else
early_putchar:
	mov   %sp,%bx
	mov   2(%bx),%al
	mov   $0x0E,%ah
	mov   $0x0007,%bx
	int   $0x10
	ret
#endif

//	Segment beginnings

// Zero for NULL pointers (near and far)
// Will be linked as first section in data segment
	.section .nildata
	.word 	0
	.word	0

	.data
	.global _endtext
	.global _endftext
	.global _enddata
	.global _endbss
	.extern	kernel_cs
	.extern	kernel_ds
	.extern task

_endtext:
	.word	0

_endftext:
	.word	0

_enddata:
	.word	0

_endbss:
	.word	0
	.bss

_sbss:
