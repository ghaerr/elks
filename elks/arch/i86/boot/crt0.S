! enable the autoconfig tool

#define __ASSEMBLY__

#include <linuxmt/config.h>
#include <arch/asm-offsets.h>

!	Assembler boot strap hooks. This is called by setup

	.text
	.globl _main
	.extern	_start_kernel
	.extern _arch_boot

! note: this next instruction is part of the kernel restart fix for
! protexted mode. it must be 3 bytes long.

	.extern _kernel_restarted
	br _kernel_restarted

!	Setup passes these on the stack
!	Setup patched to pass parameters in registers to avoid clobbering the
!	kernel when using the 286pmode extender.

_main:
#ifdef CONFIG_ROMCODE
	mov	cx,ds
	pop	bx
	pop	si
	pop	dx
#endif

! Setup.S already initialized DS and ES (but not SS)
! In addition, registers contain:
!   BX, Text size
!   SI, Data size
!   DX, BSS size
!   CX, Kernel DS
!
	mov	__endtext, bx
	mov	__enddata, si
	add	si, dx
	mov	__endbss, si

! Start cleaning BSS. Still using setup.S stack

	mov	di,__enddata	! start of BSS
	xchg	cx,dx		! CX = BSS size, DX = Kernel DS
	xor	ax,ax
	shr	cx,#1
	cld
	rep
	stosw

! End cleaning BSS

	mov	_kernel_cs, cs
	mov	_kernel_ds, ds

#ifndef CONFIG_ROMCODE
	mov	ss,dx		! in ROMCODE stack is ready placed
	mov	sp,#(_task + TASK_KSTKTOP - 10)
#endif

! Space for temporary stack space _bootstack removed!!
! Saved 768 byte boot stack.
! Print sp in wake_up and you'll see that more than 512 bytes of stack are used!
! Must be in data as its in use when we wipe the BSS

!  overwrite start of main with a jmp to kernel_restarted()
!  this will give is a call stack trace instead of the "timer bug" message
!  no longer necessary due to pmode fix. -AJB

!	.extern _redirect_main
!	call	_redirect_main

	call	_arch_boot
	call	_start_kernel	! Break point if it returns
	int	3

!	Segment beginnings

	.data
	.globl __endtext
	.globl __enddata
	.globl __endbss
	.extern	_kernel_cs
	.extern	_kernel_ds
	.extern _task

__endtext:
	.word	0

__enddata:
	.word	0

__endbss:
	.word	0
	.bss

__sbss:
