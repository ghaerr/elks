//! enable the autoconfig tool

#define __ASSEMBLY__

#include <linuxmt/config.h>
#include <arch/asm-offsets.h>

//!	Assembler boot strap hooks. This is called by setup

    .code16
	.text
	.global start
	.extern	start_kernel
	.extern arch_boot
	.global early_printk

//! note: this next instruction is part of the kernel restart fix for
//! protexted mode. it must be 3 bytes long.

//	.extern kernel_restarted
//	jmp kernel_restarted

//!	Setup passes these on the stack
//!	Setup patched to pass parameters in registers to avoid clobbering the
//!	kernel when using the 286pmode extender.

start:
/*
! Setup.S already initialized DS and ES (but not SS)
! In addition, registers contain:
!   BX, Text size
!   SI, Data size
!   DX, BSS size
!   CX, Kernel DS
!
*/
	mov	%bx,_endtext
	mov	%si,_enddata
	add	%dx,%si
	mov	%si,_endbss

//! Start cleaning BSS. Still using setup.S stack

	mov	_enddata,%di	//! start of BSS
	xchg	%dx,%cx		//! CX = BSS size, DX = Kernel DS
	xor	%ax,%ax
	shr	$1,%cx
	cld
	rep
	stosw

//! End cleaning BSS

	mov	%cs,kernel_cs
	mov	%ds,kernel_ds

	mov	%dx,%ss		//! in ROMCODE stack is ready placed
	mov	$task + TASK_USER_AX,%sp

/*
! Space for temporary stack space _bootstack removed!!
! Saved 768 byte boot stack.
! Print sp in wake_up and you'll see that more than 512 bytes of stack are used!
! Must be in data as its in use when we wipe the BSS

!  overwrite start of main with a jmp to kernel_restarted()
!  this will give is a call stack trace instead of the "timer bug" message
!  no longer necessary due to pmode fix. -AJB

!	.extern _redirect_main
!	call	_redirect_main
*/

	call	arch_boot
	call	start_kernel	//! Break point if it returns
	int	$3

early_printk:
	push  %bp
	mov   %sp,%bp
	push  %si
	mov   4(%bp),%si
	cld

ep_loop:
	lodsb
	or    %al,%al
	jz    ep_end
	mov   $0x0E,%ah
	mov   $0x0007,%bx
	int   $0x10
	jmp   ep_loop

ep_end:
	pop   %si
	pop   %bp
	ret

//!	Segment beginnings

	.data
	.global _endtext
	.global _enddata
	.global _endbss
	.extern	kernel_cs
	.extern	kernel_ds
	.extern task
	.global mfld

_endtext:
	.word	0

_enddata:
	.word	0

_endbss:
	.word	0
	.bss

_sbss:
