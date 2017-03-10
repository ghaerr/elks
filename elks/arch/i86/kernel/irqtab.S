#include <linuxmt/config.h>
#include <arch/asm-offsets.h>

	.text

/*
 *	Easy way to store our kernel DS
 *
 * moving variables from code segment to an extra segment
 * CONFIG_ROM_IRQ_DATA for the ROM_CODE-Version
 * ELKS 0.76 7/1999 Christian Mard�ller  (chm@kdt.de)
 */

#ifdef CONFIG_ROMCODE
/*
 *  In ROM-Mode we must generate a physical 3th segment :-)
 *  The segmentaddress is given by CONFIG_ROM_IRQ_DATA,
 *  the offset is constant per #define
 */
#define stashed_ds       [0]

#else
/*
 Kernel is in RAM. Reserve space in the
 code segment to save the kernel DS
*/
	.globl	stashed_ds
	.even
stashed_ds:
	.word	0
#endif

/*
 *	Low level IRQ control.
 */
	.globl	___save_flags
	.globl	_restore_flags

___save_flags:
	pushf
	pop	ax
	ret

! this version is smaller than the functionally equivalent C version
! at 7 bytes vs. 21 or thereabouts :-) --Alastair Bridgewater
!
! Further reduced to 5 bytes  --Juan Perez
!

_restore_flags:
	pop	ax
	popf
	pushf
	jmp	ax

; CS points to this kernel code segment
; DS points to page 0  (interrupt table)
; ES points to the kernel data segment

	.globl	_irqtab_init
_irqtab_init:
	cli

        mov bx,ds
#ifdef CONFIG_ROMCODE
        mov ax,#CONFIG_ROM_IRQ_DATA
        mov ds,ax
#else
        seg cs
#endif
        mov stashed_ds,bx
        mov es,bx

        xor ax,ax
        mov ds,ax      ;intr table

	mov ax,[32]
	seg es
        mov _stashed_irq0_l, ax  ; the old timer intr
	mov ax,[34]
	seg es
        mov [_stashed_irq0_l+2], ax

	mov [32],#_irq0   ;timer
	mov [34],cs

#ifndef CONFIG_CONSOLE_BIOS
        mov [36],#_irq1   ;keyboard
        mov [38],cs
#endif

#if 0	
        mov [40],#_irq2
        mov [42],cs
#endif	

        mov [44],#_irq3   ;com2
        mov [46],cs
	
	mov [48],#_irq4   ;com1
	mov [50],cs

! IRQ 8-15 are mapped to vectors INT 70h-77h

#ifdef CONFIG_ETH
	mov     [452], #_irq9
	mov     [454], cs
#endif

! Setup INT 0x80 (for syscall)
	mov [512],#_syscall_int
	mov [514],cs
! Tidy up

        mov ds,bx      ;the original value just here
	sti
	ret

/*
 *	IRQ and IRQ return paths for Linux 8086
 */
!
!	Other IRQs (see IRQ 0 at the bottom for the
!	main code).
!
_irq1:			;keyboard
	push	ax
	mov	ax,#1
	br	_irqit
#if 0
_irq2:
	push	ax
	mov	ax,#2
	br	_irqit
#endif
_irq3:                   ;com2
	push	ax
	mov	ax,#3
	br	_irqit
_irq4:                   ;com1
	push	ax
	mov	ax,#4
	br	_irqit

#if 0
_irq5:
	push	ax
	mov	ax,#5
	br	_irqit
_irq6:
	push	ax
	mov	ax,#6
	br	_irqit
_irq7:
	push	ax
	mov	ax,#7
	br	_irqit
!
!	AT interrupts
!
_irq8:
	push	ax
	mov	ax,#8
	br	_irqit
#endif

! IRQ9 is used by the Ethernet device

#ifdef CONFIG_ETH
_irq9:
	push    ax
	mov     ax, #9
	br      _irqit
#endif

#if 0
_irq10:
	push	ax
	mov	ax,#10
	br	_irqit
_irq11:
	push	ax
	mov	ax,#11
	jmp	_irqit
_irq12:
	push	ax
	mov	ax,#12
	jmp	_irqit
_irq13:
	push	ax
	mov	ax,#13
	jmp	_irqit
_irq14:
	push	ax
	mov	ax,#14
	jmp	_irqit
_irq15:
	push	ax
	mov	ax,#15
	jmp	_irqit
#endif
!
!
!	Traps (we use IRQ 16->31 for these)
!
!	Currently not used so removed for space.
#if 0
_div0:
	push	ax
	mov	ax,#16
	jmp	_irqit

_dbugtrap:
	push	ax
	mov	ax,#17
	jmp	_irqit

_nmi:
	push	ax
	mov	ax,#18
	jmp	_irqit

_brkpt:
	push	ax
	mov	ax,#19
	jmp	_irqit

_oflow:
	push	ax
	mov	ax,#20
	jmp	_irqit

_bounds:
	push	ax
	mov	ax,#21
	jmp	_irqit

_invop:
	push	ax
	mov	ax,#22
	jmp	_irqit

_devnp:
	push	ax
	mov	ax,#23
	jmp	_irqit

_dfault:
	push	ax
	mov	ax,#24
	jmp	_irqit
;
;	trap 9 is reserved
;
_itss:
	push	ax
	mov	ax,#26
	jmp	_irqit

_nseg:
	push	ax
	mov	ax,#27
	jmp	_irqit

_stkfault:
	push 	ax
	mov	ax,#28
	jmp	_irqit

_segovr:
	push	ax
	mov	ax,#29
	jmp	_irqit

_pfault:
	push	ax
	mov	ax,#30
	jmp	_irqit
;
;	trap 15 is reserved
;
_fpetrap:
	push	ax
	mov	ax,#32
	jmp	_irqit

_algn:
	push	ax
	mov	ax,#33
	jmp	_irqit

#endif
_syscall_int:
	push	ax
	mov	ax,#0x80
	jmp	_irqit
!
!	On entry CS:IP is all we can trust
!
!	There are three possible cases to cope with
!
!	SS = kernel DS.
!		Interrupted kernel mode code or kernel task
!		No task switch allowed
!		Running on a kernel process stack anyway.
!
!	SS = current->t_regs.ss
!		Interrupted user mode code or syscall
!		Switch to kernel stack for process (will be free)
!		Task switch allowed
!
!	Other
!		BIOS or other 'strange' code.
!		Must be called from kernel space, but kernel stack is in use
!		Switch to int_stack
!		No task switch allowed.
!
!	We do all of this to avoid per process interrupt stacks and
!	related nonsense. This way we need only one dedicted int stack
!
!  ELKS 0.76 7/1999  Fixed for ROMCODE-Version
!  Christian Mardm�ller  (chm@kdt.de)
!
	.globl	_ret_from_syscall
	.extern	_schedule
	.extern	_do_signal
	.extern	_do_IRQ
	.extern	_stack_check
	.extern	_syscall
#ifdef CONFIG_STRACE
	.extern	_strace
	.extern	_ret_strace
#endif

_irq0:
!
!	Save AX and load it with the IRQ number
!
	push	ax
	xor	ax,ax
_irqit:
!
!	Make room
!
	push	ds
	push	si
	push	di
!
!	Recover data segment
!
#ifdef CONFIG_ROMCODE
	mov	si,#CONFIG_ROM_IRQ_DATA
	mov	ds,si
	mov	ds,stashed_ds
#else
	seg	cs
	mov	ds,stashed_ds
#endif
!
!	See where we were
!
	mov	di,ss		! Get current SS
	cmp	di,_kernel_ds	! SS = kernel SS ?
	je	ktask		! Kernel - no work
!
!	User or BIOS etc
!
	mov	si,_current
	cmp	di,TASK_USER_SS[si] ! entry SS = current->t_regs.ss?
	jne	btask		! Switch to interrupt stack
!
!	User mode case - switch to kernel stack
!
	add	si,#TASK_USER_DI
	j	save_regs
!
!	Bios etc - switch to interrupt stack
!
btask:
	mov	si,#(_intstack-12)
	j	save_regs
!
!	Kernel mode case - keep using kernel stack
!
ktask:
	mov	si,sp
	sub	si,#6
!
!	Save segment, index, BP and SP registers
!
save_regs:
	pop	[si]		! DI
	pop	2[si]		! SI
	pop	6[si]		! DS
	pop	di		! Original AX
	push	bp		! BP
	mov	8[si],sp	! SP
	mov	10[si],ss	! SS
	mov	4[si],es	! ES
!
!	Load new segment and SP registers
!
	mov	sp,si
	mov	si,ds
	mov	ss,si
	mov	es,si
!
!	Save remaining registers
!
	push	dx		! DX
	push	cx		! CX
	push	bx		! BX
	push	di		! AX
!
!	AX has interrupt number
!
	cmp	ax,#0x80
	jne	updct
!
!	----------PROCESS SYSCALL----------
!
	sti
	call	_stack_check	! Check USER stack
	pop	ax		! Get syscall function code
#ifdef CONFIG_STRACE
!
!	strace(syscall#, params...)
!
	push	ax
	call	_strace
	pop	ax
#endif
!
!	syscall(params...)
!
	call	_syscall
	push	ax		! syscall returns a value in ax
	call	_do_signal
#ifdef CONFIG_STRACE
!
!	ret_strace(retval)
!
	pop	ax
	push	ax
	call	_ret_strace
#endif
!
!	Restore registers
!
	cli
	j	restore_regs
!
!	Done.
!
_ret_from_syscall:
	mov	bx,_current	! Ensure we have the
	lea	sp,TASK_USER_BX[bx] ! right kernel SP
	xor	ax,ax		! Just in case we are starting a new task
	push	ax
	cli
	j	restore_regs
!
!	----------PROCESS INTERRUPT----------
!
!	Update intr_count
!
updct:
	inc	_intr_count
!
!	Call the C code
!
	sti			! Reenable interrupts
	mov	bx,sp		! Get pointer to pt_regs
	push	ax		! IRQ for later

	push	bx		! Register base
	push	ax		! IRQ number
	call	_do_IRQ		! Do the work
	pop	ax		! Clean parameters
	pop	bx

	pop	ax		! Saved IRQ
!
!	Send EOI to interrupt controller
!
	cli			! Disable interrupts to avoid reentering ISR
	cmp	ax,#16
	jge	was_trap	! Traps need no reset
	or	ax,ax		! Is int #0?
	jnz	a4
!
!	IRQ 0 (timer) has to go on to the bios for some systems
!
	dec	_bios_call_cnt_l ! Will call bios int?
	jne	a4
	mov	_bios_call_cnt_l,#5
	pushf
	callf	[_stashed_irq0_l]
	jmp	was_trap	! EOI already sent by bios int
a4:
	cmp	ax,#8
	movb	al,#0x20	! EOI
	jb	a6		! IRQ on low chip
!
!	Reset secondary 8259 if we have taken an AT rather
!	than XT irq. We also have to prod the primay
!	controller EOI..
!
	outb	0xA0,al
	jmp	a5
a5:	jmp	a6
a6:	outb	0x20,al		! Ack on primary controller
!
!	And a trap does no hardware work
!
was_trap:
!
!	Restore intr_count
!
	dec	_intr_count
!
!	Now look at rescheduling
!
	mov	bx,_current	! Schedule allowed ?
	add	bx,#TASK_USER_AX
	cmp	bx,sp
	jne	restore_regs	! No
!	cmp	_need_resched,#0 ! Schedule needed ?
!	je	restore_regs	! No
!
! This path will return directly to user space
!
	call	_schedule	! Task switch
	call	_do_signal	! Check signals
!
!	Restore registers and return
!
restore_regs:
	pop	ax
	pop	bx
	pop	cx
	pop	dx
	pop	di
	pop	si
	pop	es
	pop	ds
	pop	bp
	pop	ss
	mov	sp,bp
	pop	bp
!
!	Iret restores CS:IP and F (thus including the interrupt bit)
!
	iret

/*
 *	tswitch();
 *
 *	This function can only be called with SS=DS=ES=kernel DS and
 *	CS=kernel CS. SS:SP is the relevant kernel stack. Thus we don't need
 *	to arse about with segment registers. The kernel isn't relocating.
 *
 *	tswitch() saves the "previous" task registers and state. It in effect
 *	freezes a copy of the caller context. Then restores the "current"
 *	context and returns running the current task.
 */

	.globl	_tswitch

_tswitch:
	push	bp		! schedule()'s bp
	pushf
	push	di
	push	si
	mov	bx,_previous
	mov	TASK_KRNL_SP[bx],sp
	mov	bx,_current
	mov	sp,TASK_KRNL_SP[bx]
	pop	si
	pop	di
	popf
	pop	bp		! BP of schedule()
	ret
!
	.data
	.globl	_intr_count
	.extern	_current
	.extern	_previous
	.extern	_kernel_ds

	.even

_bios_call_cnt_l:
	.word	5
_stashed_irq0_l:
	.long	0
_intr_count:
	.word	0

	.zerow	256		! (was) 128 byte interrupt stack
_intstack:

