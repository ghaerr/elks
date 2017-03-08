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
!		Interrupted user mode code
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
!	Save all registers
!
	push	ds
	push	es
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	bp
!
!	Recover data segment
!
#ifdef CONFIG_ROMCODE
	mov	si,#CONFIG_ROM_IRQ_DATA
	mov	ds,si
	mov	si,stashed_ds
#else
	seg	cs
	mov	si,stashed_ds
#endif
	mov	ds,si
	mov	es,si

!
!	Set up task switch controller
!
	xor	ch,ch		! Assume we are not allowed to switch
!
!       See where we were (DX holds the SS on entry)
!
	mov	di,ss		! Get current SS
	cmp	di,si		! SS = kernel SS ?
	je	ktask		! Kernel - no work
!
!	User or BIOS etc
!
	mov	ss,si		! Set SS: right
	mov	si,_current
	cmp	di,TASK_USER_SS[si] ! entry ss = current->t_regs.ss?
	jne	btask		! Switch to interrupt stack
!
!	User task. Extract kernel SP. (BX already holds current)
!	At this point, the kernel stack is empty. Thus, we can load
!       the kernel stack pointer without accesing memory
!
	mov	TASK_USER_SP[si],sp
	lea	sp,TASK_USER_AX[si] ! switch to kernel stack ptr
	lea	bp,TASK_USER_SP[si]
	inc	ch		! Switch allowable
        j       updct
!
!	Bios etc - switch to interrupt stack
!
btask:
	mov	sp,#_intstack
!
!	In ktask state we have a suitable stack. It might be
!	better to use the intstack..
!
ktask:
!
!	Put the old SS;SP on the top of the stack. We can't
!	leave them in stashed_ss/sp as we could re-enter the
!	routine on a reschedule.
!
	mov	si,sp		! Get current SP
	push	di		! push entry SS
	push	si		! push entry SP
!
!	The registers are now stored. Remember where
!
	mov	bp,sp
!
!	Update intr_count
!
updct:
	inc	_intr_count
!
!	Call the C code
!
	sti			! Reenable interrupts
	push	cx		! Switch flag
	push	ax		! IRQ for later

	push	bp		! Register base
	push	ax		! IRQ number
	call	_do_IRQ		! Do the work
	pop	ax		! Clean parameters
	pop	bx

	pop	ax		! Saved IRQ
	pop	cx		! Recover switch allowed flag
!
!	Restore any chips
!
        cli                     ! Disable interrupts to avoid reentering ISR
	cmp	ax,#16
	jge	was_trap	! Traps need no reset
	or	ax,ax		! Is int #0?
	jnz	a4
!
!        IRQ 0 (timer) has to go on to the bios for some systems
!
        dec     _bios_call_cnt_l ! Will call bios int?
        jne     a4
        mov     _bios_call_cnt_l,#5
        pushf
        callf   [_stashed_irq0_l]
	jmp	was_trap	! EOI already sent by bios int
a4:
        cmp     ax,#8
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
	orb	ch,ch			! Schedule allowed ?
	je	nosched			! No
!	mov	bx,_need_resched	! Schedule needed
!	cmp	bx,#0			!
!	je	nosched			! No
!
! This path will return directly to user space
!
	call	_schedule		! Task switch
	mov	bx,_current
	mov	TASK_USER_FL[bx],#1
	call	_do_signal		! Check signals
!
!	At this point, the kernel stack is empty. Thus, there is no
!       need to save the kernel stack pointer.
!
	mov	bx,_current
	mov	sp,TASK_USER_SP[bx]
	mov	ss,TASK_USER_SS[bx]	! user ds
#ifdef CONFIG_ADVANCED_MM
	mov	bp,sp
	mov	12[bp],ss		! change the es in the stack
	mov	14[bp],ss		! change the ds in the stack
#endif
	j	noschedpop
!
!	Now we have to rescue our stack pointer/segment.
!
nosched:
	pop	cx	! SP
	pop	ss	! SS
	mov	sp,cx
!
!	Restore registers and return
!
noschedpop:
	pop	bp
	pop 	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	es
	pop	ds
	pop	ax
!
!	Iret restores CS:IP and F (thus including the interrupt bit)
!
	iret

/*
 *	tswitch();
 *
 *	This function can only be called with SS=DS=ES=kernel DS and
 *	CS=kernel CS. SS:SP is the relevant kernel stack (IRQ's are
 *	taken on 'current' kernel stack). Thus we don't need to arse about
 *	with segment registers. The kernel isn't relocating.
 *
 *	tswitch() saves the "previous" task registers and state. It in effect
 *	freezes a copy of the caller context. Then restores the "current"
 *	context and returns running the current task.
 */

	.globl _tswitch

_tswitch:
	push	bp	! /* schedule()'s bp */
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
	pop	bp	! BP of schedule()
	xor	ax,ax	! Set ax=0, as this may be fork() return from child
	ret		! thus to caller of schedule()

!
!	System Call Vector
!
!	On entry we are on the wrong stack, DS, ES are wrong
!
!	System calls enter here with ax as function and bx,cx,dx,di and si
!	as parameters.
!	syscall returns a value in ax
!

_syscall_int:
!
!	We know the process DS, we can discard it (indeed may change it)
!
!	Save si and free an index register
!
	push	si
!
!	Load kernel data segment
!
#ifdef CONFIG_ROMCODE
	mov	si,#CONFIG_ROM_IRQ_DATA
	mov	ds,si
#else
	seg	cs
#endif
	mov	ds,stashed_ds		! the org DS of kernel
!
!	At this point, the kernel stack is empty. Thus, we can push
!	data into the kernel stack by writing directly to memory
!
	mov	si,_current
	add	si,#TASK_USER_SI	! pops SI from user stack and pushes
	pop	[si]			! it directly into kernel stack
	push	bp			! Save BP in the user stack
!
!	Stash user mode stack - needed for stack checking!
!
	mov	6[si],sp		! Save user SP
!
!	load kernel stack pointer
!
	mov	sp,si			! Load kernel SP
!
!	Finish switching to the right things
!
	mov	si,ds			! ds=es=ss
	mov	es,si
	mov	ss,si
	cld
!
!	Stack is now right, we can take interrupts OK
!
	sti				! SI already on top of stack
	push	di
	push	dx
	push	cx
	push	bx

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
	push	ax
	call	_stack_check
	pop	ax
	call	_syscall
	push	ax
	mov	bx,_current
	mov	TASK_USER_FL[bx],#0
	call	_do_signal
	pop	ax
_ret_from_syscall:
	mov	bx,_current		! Ensure we have the
	lea	sp,TASK_USER_BX[bx]	! right kernel SP
#ifdef CONFIG_STRACE
!
!	ret_strace(retval)
!
	push	ax
	call	_ret_strace
	pop	ax
#endif
!
!	Restore registers
!
	cli
	pop	bx
	pop	cx
	pop	dx
	pop	di
	pop	si
	pop	es
	pop	ds
	pop	bp			! Restore user SS:SP
	pop	ss
	mov	sp,bp
	pop	bp			! Restore user BP
!
!	return with error info.
!
	iret
!
!	Done.
!

	.data
	.globl	_intr_count
	.extern	_current
	.extern	_previous

	.even

_bios_call_cnt_l:
	.word	5
_stashed_irq0_l:
	.long	0
_intr_count:
	.word	0

	.zerow	256		! (was) 128 byte interrupt stack
_intstack:

