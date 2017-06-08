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

#ifdef CONFIG_CONSOLE_DIRECT
#define MCD 0x2
#else
#define MCD 0
#endif
#ifdef CONFIG_CHAR_DEV_RS
#define MRS 0x18
#else
#define MRS 0
#endif
#ifdef CONFIG_BLK_DEV_HD
#define MHD 0xC020
#else
#define MHD 0
#endif
#ifdef CONFIG_BLK_DEV_FD
#define MFD 0x40
#else
#define MFD 0
#endif
#ifdef CONFIG_ETH_NE2K
#define METH 0x200
#else
#define METH 0
#endif

#define MINT (1 | MCD | MRS | MHD | MFD | METH)

	.globl	_irqtab_init
_irqtab_init:
	cli
#ifdef CONFIG_ROMCODE
	mov	ax,#CONFIG_ROM_IRQ_DATA
	mov	ds,ax
#else
	seg	cs
#endif
	mov	stashed_ds,ss

	xor	ax,ax
	mov	ds,ax

! CS points to this kernel code segment
! DS points to page 0  (interrupt table)
! ES points to the kernel data segment

	mov	[512],#_syscall_int ! syscall
	mov	[514],cs

	mov	bx,#32
	mov	ax,[bx]		! get the old timer intr
	seg	es
	mov	_stashed_irq0_l,ax
	mov	ax,2[bx]
	seg	es
	mov	[_stashed_irq0_l+2],ax

	mov	cx,#MINT
	mov	ax,#_irq0
init_tab1:
	shr	cx,#1
	jnc	init_tab2
	mov	[bx],ax
	mov	2[bx],cs
	add	ax,#4
init_tab2:
	add	bx,#4
	cmp	bx,#64
	jne	init_tab3
	add	bx,#384		! IRQ 8-15 are mapped to vectors INT 70h-77h
init_tab3:
	or	cx,cx
	jnz	init_tab1
!
! Tidy up
!
	push	ss		! restore DS
	pop	ds
	sti
	ret

!	IRQ and IRQ return paths for Linux 8086
!
! The execution thread will not return from the function call.
! Instead, the address pushed in the stack will be used to get
! the interrupt number.

_irq0:			! Timer
	call	_irqit
	.byte	0
#ifdef CONFIG_CONSOLE_DIRECT
_irq1:			! Keyboard
	call	_irqit
	.byte	1
#endif
#if 0
_irq2:			! Cascade
	call	_irqit
	.byte	2
#endif
#ifdef CONFIG_CHAR_DEV_RS
_irq3:			! COM2
	call	_irqit
	.byte	3
_irq4:			! COM1
	call	_irqit
	.byte	4
#endif
#ifdef CONFIG_BLK_DEV_HD
_irq5:			! XT HD
	call	_irqit
	.byte	5
#endif
#ifdef CONFIG_BLK_DEV_FD
_irq6:			! Floppy
	call	_irqit
	.byte	6
#endif
#if 0
_irq7:			! Lp1
	call	_irqit
	.byte	7
!
!	AT interrupts
!
_irq8:			! RTC
	call	_irqit
	.byte	8
#endif
#ifdef CONFIG_ETH_NE2K
_irq9:			! Ethernet device
	call	_irqit
	.byte	9
#endif
#if 0
_irq10:			! USB
	call	_irqit
	.byte	10
_irq11:			! Sound
	call	_irqit
	.byte	11
_irq12:			! Mouse
	call	_irqit
	.byte	12
_irq13:			! Math coproc.
	call	_irqit
	.byte	13
#endif
#ifdef CONFIG_BLK_DEV_HD
_irq14:			! AT HD ide primary
	call	_irqit
	.byte	14
_irq15:			! AT HD ide secondary
	call	_irqit
	.byte	15
#endif
!
!	Traps (we use IRQ 16->31 for these)
!
!	Currently not used so removed for space.
#ifdef ENABLE_TRAPS
_div0:
	call	_irqit
	.byte	16
_dbugtrap:
	call	_irqit
	.byte	17
_nmi:
	call	_irqit
	.byte	18
_brkpt:
	call	_irqit
	.byte	19
_oflow:
	call	_irqit
	.byte	20
_bounds:
	call	_irqit
	.byte	21
_invop:
	call	_irqit
	.byte	22
_devnp:
	call	_irqit
	.byte	23
_dfault:
	call	_irqit
	.byte	24
;
;	trap 9 is reserved
;
_itss:
	call	_irqit
	.byte	26
_nseg:
	call	_irqit
	.byte	27
_stkfault:
	call	_irqit
	.byte	28
_segovr:
	call	_irqit
	.byte	29
_pfault:
	call	_irqit
	.byte	30
;
;	trap 15 is reserved
;
_fpetrap:
	call	_irqit
	.byte	32
_algn:
	call	_irqit
	.byte	33
#endif
_syscall_int:		! Syscall
	call	_irqit
	.byte	128

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
!	related nonsense. This way we need only one dedicated int stack
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
	pop	di		! Pointer to interrupt number
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
	push	ax		! AX
!
!	cs:[di] has interrupt number
!
	seg	cs
	movb	al,[di]
	cmpb	al,#0x80
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
#ifdef CONFIG_STRACE
!
!	ret_strace(retval)
!
	call	_ret_strace
#endif
!
!	Restore registers
!
	call	_do_signal
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
	cbw
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
#ifdef USE_IA16
	push	es
#endif
	push	di
	push	si
	mov	bx,_previous
	mov	TASK_KRNL_SP[bx],sp
	mov	bx,_current
	mov	sp,TASK_KRNL_SP[bx]
	pop	si
	pop	di
#ifdef USE_IA16
	pop	es
#endif
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

