#include <linuxmt/config.h>
#include <arch/irq.h>
#include <arch/asm-offsets.h>

/*
 *	Easy way to store our kernel DS
 *
 * moving variables from code segment to an extra segment
 * CONFIG_ROM_IRQ_DATA for the ROM_CODE-Version
 * ELKS 0.76 7/1999 Christian Mard”ller  (chm@kdt.de)
 */

#ifdef CONFIG_ROMCODE
/*
 *  In ROM-Mode we must generate a physical 3th segment :-)
 *  The segmentaddress is given by CONFIG_ROM_IRQ_DATA,
 *  the offset is constant per #define
 */
   #define stashed_ds       [0]
#endif

#ifndef S_SPLINT_S
#asm

	.text

#ifndef CONFIG_ROMCODE
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
	.globl ___save_flags
	.globl _restore_flags

___save_flags:
	pushf
	pop ax
	ret

! this version is smaller than the functionally equivalent C version
! at 7 bytes vs. 21 or thereabouts :-) --Alastair Bridgewater
!
! Further reduced to 5 bytes  --Juan Perez
!

_restore_flags:
	pop ax
	popf
	pushf
	jmp ax

; CS points to this kernel code segment
; ES points to page 0  (interrupt table)
; DS points to the irqdataseg (cs or CONFIG_ROM_IRQ_DATA)

	.globl _irqtab_init
_irqtab_init:
	mov	al, #0x00	! disable psion hardware interrupt sources
	out	0x15, al
	mov	al, #0x00
	out	0x08, al
        cli

	mov	bx,ds
#ifdef CONFIG_ROMCODE
	mov	ax,#CONFIG_ROM_IRQ_DATA
	mov	ds,ax
#else
	seg	cs
#endif
	mov	stashed_ds,bx
	mov	es,bx

        xor ax,ax
	mov	ds,ax	;intr table

	out	0x15, al	! memory protection
	mov	[0x01e6],cs
	mov	[0x01e4],#_irq0
#if 0
	mov	ax, cs
	seg	es
	mov	0x01c2, ax
#endif
#if 0
	mov	[0xe4],#_irq0
	mov	[0x02],cs

	mov	[0x04],#_irq1
	mov	[0x06],cs

	mov	[0x08],#_irq2
	mov	[0x0A],cs

	mov	[0x0C],#_irq3
	mov	[0x0E],cs

	mov	[0x10],#_irq4
	mov	[0x12],cs

#endif

! Setup INT 0x80 (for syscall)
	mov	[512],#_syscall_int
	mov	[514],cs
! Tidy up

	mov	ds,bx	;the original value just here
	sti
	ret

/*
 *	IRQ and IRQ return paths for Linux 8086
 */
!
!	Other IRQs (see IRQ 0 at the bottom for the
!	main code).
!
	.extern	_schedule
	.extern	_do_signal
	.extern	_do_IRQ

_irq1:
	push	ax
	mov	ax,#1
	br	_irqit
_irq2:
	push	ax
	mov	ax,#2
	br	_irqit
_irq3:
	push	ax
	mov	ax,#3
	br	_irqit

_irq4:
	push	ax
	mov	ax,#4
	br	_irqit

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
_irq8:
	push	ax
	mov	ax,#8
	br	_irqit
_irq9:
	push	ax
	mov	ax,#9
	br	_irqit
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
	push 	ax
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
!		Interrupted kernel mode code.
!		No task switch allowed
!		Running on a kernel process stack anyway.
!
!	SS = current->user_ss
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
!
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
!	Recover segments
!
#ifdef CONFIG_ROMCODE
        mov bx,#CONFIG_ROM_IRQ_DATA
        mov ds,bx
#else
	seg	cs
#endif
	mov	bx,stashed_ds		! Recover the data segment
	mov	ds,bx
	mov	es,bx

	mov	dx,ss			! Get current SS
	mov	bp,sp			! Get current SP
!
!	Set up task switch controller
!
	xor	ch,ch		! Assume we are not allowed to switch
!
!	See where we were (BX holds the SS on entry)
!
	cmp	dx,bx		! SS = kernel SS ?
	je	ktask		! Kernel - no work
!
!	User or BIOS etc
!
        mov     ss,bx           ! /* Set SS: right */
	mov	bx,_current
	cmp	dx,TASK_USER_SS[bx] ! entry ss = current->t_regs.ss?
	jne	btask		! Switch to interrupt stack
!
!	User task. Extract kernel SP. (BX already holds current)
!	At this point, the kernel stack is empty. Thus, we can load
!       the kernel stack pointer without accesing memory
!
        mov     TASK_USER_SP[bx],sp
        lea     sp,TASK_KSTKTOP[bx] ! switch to kernel stack ptr
        lea     bp,TASK_USER_SP[bx]
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
! /*
!	Put the old SS;SP on the top of the stack. We can't
!	leave them in stashed_ss/sp as we could re-enter the
!	routine on a reschedule.
! */
	push	dx		! push entry SS
	push	bp		! push entry SP
!
!	The registers are now stored. Remember where
!
	mov	bp,sp
!
!   Update intr_count
!
updct:
        inc     _intr_count
!
!       We are on a suitable stack and ch says whether
!       we can switch afterwards.
!
        push    cx              ! Switch flag
	push	ax		! IRQ for later
	push	bp		! Register base
	push	ax		! IRQ number
!
!	Call the C code
!
        call    _do_IRQ         ! Do the work. Interrupt handler should enable
!                                 interrupts after removing interrupt signal
!	Return path
!
	pop	ax		! We want the ax value back
	pop	bx		! Drop arguments
	pop	ax		! Saved IRQ
	pop	cx		! Recover switch allowed flag
!
!	Restore any chips
!
        cli                     ! Disable interrupts to avoid reentering ISR
!
! The individual IRQ now reset as the Psion has a weird structure
!
!   Restore intr_count
!
        dec     _intr_count
!
!	Now look at rescheduling
!
        orb     ch,ch                   ! Schedule allowed ?
	je	nosched			! No
!	mov	bx,_need_resched	! Schedule needed
!	cmp	bx,#0			!
!	je	nosched			! No
!
! This path will return directly to user space
!
	call	_schedule		! Task switch
        mov     bx,_current
        mov     8[bx],#1
        call    _do_signal              ! Check signals
!
!	At this point, the kernel stack is empty. Thus, there in no
!       need to save the kernel stack pointer.
!
        mov     bx,_current
        mov     sp,TASK_USER_SP[bx]
#ifndef CONFIG_ADVANCED_MM
        mov     ss,TASK_USER_SS[bx]
#else
	mov ax, TASK_USER_SS[bx] ! user ds
	mov bp, sp
        mov ss, ax
	mov 12[bp], ax	! change the es in the stack
	mov 14[bp], ax	! change the ds in the stack
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

	.data
	.globl	_intr_count

	.even

off_stashed_irq0_l:
	.word	0
seg_stashed_irq0_l:
	.word	0
_intr_count:
	.word	0

	.zerow	256		! (was) 128 byte interrupt stack
_intstack:

#endasm
#endif
