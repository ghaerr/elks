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

#ifdef REL_SYS
! Relocate kernel code and data to lower memory
! First set the stack next to the kernel data segment

	mov	ax,si
	add	ax,dx
	rcr	ax,#1
	shr	ax,#1
	shr	ax,#1
	shr	ax,#1
	add	ax,cx
	mov	ss,ax
	mov	sp,#768
	push	dx		! Save registers
	push	si
	push	cx
	push	bx

! Relocate setup data

	mov	ax,#DEF_INITSEG
	mov	ds,ax
	mov	ax,#REL_SETUPSEG
	mov	es,ax
	xor	si,si
	xor	di,di
	mov	cx,#256
	cld
	rep
	movsw

! Relocate 1Kb of kernel code segment

	mov	dx,cs		! Make sure this code
	mov	ds,dx		! gets relocated
	mov	ax,#REL_SYSSEG
	mov	es,ax
	sub	dx,ax
	xor	si,si
	xor	di,di
	mov	cx,#512
	rep
	movsw
	jmpi	cont,REL_SYSSEG

! Relocate the remaining kernel code

cont:	pop	bx		! Don't worry if copy
	mov	cx,bx		! overlaps
	sub	cx,#1024
	rep
	movsb

! Relocate the kernel data segment

	pop	ax
	mov	ds,ax
	sub	ax,dx
	mov	es,ax		! New kernel DS
	xor	si,si
	xor	di,di
	pop	ax
	mov	cx,ax
	rep
	movsb
	mov	si,ax
	mov	cx,es
	mov	ds,cx
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
	mov	sp,#(_task + TASK_USER_AX)
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
