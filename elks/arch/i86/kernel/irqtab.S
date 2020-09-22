#include <linuxmt/config.h>
#include <arch/asm-offsets.h>
#include <arch/ports.h>

#define CHECK_SS	// integrity check interrupt from user mode

	.code16

	.text

/*
 *	Easy way to find the kernel DS
 *
 * Define a variable in the code segment whose
 * predefined contents is the kernel data segment
 */

	.align 1

// FIXME: data in code segment - move to INITSEG
	.global	ds_kernel
ds_kernel:
#if defined(CONFIG_ROMCODE)
	.word	CONFIG_ROM_KERNEL_DATA
#else
	.word	0
#endif

/* build MINT mask based on required interrupts in arch/ports.h*/
#ifdef CONFIG_NEED_IRQ0
#define M0	0x01
#else
#define M0	0
#endif
#ifdef CONFIG_NEED_IRQ1
#define M1	0x02
#else
#define M1	0
#endif
#ifdef CONFIG_NEED_IRQ2
#define M2	0x04
#else
#define M2	0
#endif
#if defined(CONFIG_NEED_IRQ3) || defined (CONFIG_FAST_IRQ3)
#define M3	0x08
#else
#define M3	0
#endif
#if defined(CONFIG_NEED_IRQ4) || defined (CONFIG_FAST_IRQ4)
#define M4	0x10
#else
#define M4	0
#endif
#ifdef CONFIG_NEED_IRQ5
#define M5	0x20
#else
#define M5	0
#endif
#ifdef CONFIG_NEED_IRQ6
#define M6	0x40
#else
#define M6	0
#endif
#ifdef CONFIG_NEED_IRQ7
#define M7	0x80
#else
#define M7	0
#endif
#ifdef CONFIG_NEED_IRQ8
#define M8	0x100
#else
#define M8	0
#endif
#ifdef CONFIG_NEED_IRQ9
#define M9	0x200
#else
#define M9	0
#endif
#ifdef CONFIG_NEED_IRQ10
#define M10	0x400
#else
#define M10	0
#endif
#ifdef CONFIG_NEED_IRQ11
#define M11	0x800
#else
#define M11	0
#endif
#ifdef CONFIG_NEED_IRQ12
#define M12	0x1000
#else
#define M12	0
#endif
#ifdef CONFIG_NEED_IRQ13
#define M13	0x2000
#else
#define M13	0
#endif
#ifdef CONFIG_NEED_IRQ14
#define M14	0x4000
#else
#define M14	0
#endif
#ifdef CONFIG_NEED_IRQ15
#define M15	0x8000
#else
#define M15	0
#endif

/* took awhile to generate this...*/
#define MINT (M0|M1|M2|M3|M4|M5|M6|M7|M8|M9|M10|M11|M12|M13|M14|M15)

	.global	irqtab_init
irqtab_init:
	cli

#if !defined(CONFIG_ROMCODE)
	mov	%ss,%cs:ds_kernel
#endif

	xor	%ax,%ax
	mov	%ax,%ds

// CS points to this kernel code segment
// DS points to page 0  (interrupt table)
// ES points to the kernel data segment

	movw	$_syscall_int,512 // syscall
	mov	%cs,514

	mov	$32,%bx
	mov	(%bx),%ax		// get the old timer intr
//	seg	es
	mov	%ax,%es:_stashed_irq0_l
	mov	2(%bx),%ax
//	seg	es
	mov	%ax,%es:_stashed_irq0_l+2

	mov	$MINT,%cx
	mov	$_irqtable,%ax
init_tab1:
	shr	$1,%cx
	jnc	init_tab2
	mov	%ax,(%bx)
	mov	%cs,2(%bx)
	add	$4,%ax
init_tab2:
	add	$4,%bx
	cmp	$64,%bx
	jne	init_tab3
	add	$384,%bx		// IRQ 8-15 are mapped to vectors INT 70h-77h
init_tab3:
	or	%cx,%cx
	jnz	init_tab1
//
// Tidy up
//
	push	%ss		// restore DS
	pop	%ds
	sti
	ret

//	IRQ and IRQ return paths for Linux 8086
//
// The execution thread will not return from the function call.
// Instead, the address pushed in the stack will be used to get
// the interrupt number.

_irqtable:
#ifdef CONFIG_NEED_IRQ0
_irq0:			// Timer
	call	_irqit
	.byte	0
#endif
#ifdef CONFIG_NEED_IRQ1
_irq1:			// Keyboard
	call	_irqit
	.byte	1
#endif
#ifdef CONFIG_NEED_IRQ2
_irq2:			// Cascade
	call	_irqit
	.byte	2
#endif
#if defined(CONFIG_NEED_IRQ3) || defined(CONFIG_FAST_IRQ3)
_irq3:			// COM2
#ifdef CONFIG_FAST_IRQ3
	.global _irq_com2	// fastest possible interrupt routine, fixed com2 port
	jmp	_irq_com2
	.byte	3		// ignored
#endif
#ifdef CONFIG_NEED_IRQ3
	call	_irqit		// original ELKS overhead interrupt routine
	.byte	3
#endif
#endif
#if defined(CONFIG_NEED_IRQ4) || defined(CONFIG_FAST_IRQ4)
_irq4:			// COM1
#ifdef CONFIG_FAST_IRQ4
	.global _irq_com1	// fastest possible interrupt routine, fixed com1 port
	jmp	_irq_com1
	.byte	4		// ignored
#endif
#ifdef CONFIG_NEED_IRQ4
	call	_irqit		// original ELKS overhead interrupt routine
	.byte	4
#endif
#endif
#ifdef CONFIG_NEED_IRQ5
_irq5:			// XT HD
	call	_irqit
	.byte	5
#endif
#ifdef CONFIG_NEED_IRQ6
_irq6:			// Floppy
	call	_irqit
	.byte	6
#endif
#ifdef CONFIG_NEED_IRQ7
_irq7:			// Lp1
	call	_irqit
	.byte	7
#endif

//
//	AT interrupts
//

#ifdef CONFIG_NEED_IRQ8
_irq8:			// RTC
	call	_irqit
	.byte	8
#endif
#ifdef CONFIG_NEED_IRQ9
_irq9:			// Ethernet device
	call	_irqit
	.byte	9
#endif
#ifdef CONFIG_NEED_IRQ10
_irq10:			// USB
	call	_irqit
	.byte	10
#endif
#ifdef CONFIG_NEED_IRQ11
_irq11:			// Sound
	call	_irqit
	.byte	11
#endif
#ifdef CONFIG_NEED_IRQ12
_irq12:			// Mouse
	call	_irqit
	.byte	12
#endif
#ifdef CONFIG_NEED_IRQ13
_irq13:			// Math coproc.
	call	_irqit
	.byte	13
#endif
#ifdef CONFIG_NEED_IRQ14
_irq14:			// AT HD ide primary
	call	_irqit
	.byte	14
#endif
#ifdef CONFIG_NEED_IRQ15
_irq15:			// AT HD ide secondary
	call	_irqit
	.byte	15
#endif
//
//	Traps (we use IRQ 16->31 for these)
//
//	Currently not used so removed for space.
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
_syscall_int:		// Syscall
	call	_irqit
	.byte	128

/*
!
!	On entry CS:IP is all we can trust
!
!	There are three possible cases to cope with
!
!	Interrupted user mode or syscall (_gint_count == 0)
!		Switch to process's kernel stack
!		Optionally, check (SS == current->t_regs.ss)
!		and panic on failure
!		On return, task switch allowed
!
!	Interrupted kernel mode, interrupted kernel task
!		or second interrupt (_gint_count == 1)
!		Switch to interrupt stack
!		On return, no task switch allowed
!
!	Interrupted interrupt service routine (_gint_count > 1)
!		Already using interrupt stack, keep using it
!		On return, no task switch allowed
!
!	We do all of this to avoid per process interrupt stacks and
!	related nonsense. This way we need only one dedicated int stack
!
*/
	.global	ret_from_syscall
	.extern	schedule
	.extern	do_signal
	.extern	do_IRQ
	.extern	stack_check
	.extern	syscall
#ifdef CHECK_SS
	.extern	panic
#endif
#ifdef CONFIG_STRACE
	.extern	strace
	.extern	ret_strace
#endif

_irqit:
//
//	Make room
//
	push	%ds
	push	%si
	push	%di
//
//	Recover data segment
//
//	seg	cs
	mov	%cs:ds_kernel,%ds
//
//	Determine which stack to use
//
	cmpw	$1,_gint_count
	jc	utask		// We were in user mode
	jz	itask		// Using a process's kernel stack
ktask:				// Already using interrupt stack
//
//	Already using interrupt stack, keep using it
//
	mov	%sp,%si
	sub	$8,%si		// 14 offsets less 6 already on stack
	jmp	save_regs
//
//	Using a process's kernel stack, switch to interrupt stack
//
itask:
	mov	$_intstack-14,%si // 14 offsets 0-13 of SI below
	jmp	save_regs
//
//	User mode case
//
utask:
	mov	current,%si
#ifdef CHECK_SS
//
//	We were in user mode, first confirm
//
	mov	%ss,%di
	cmp	TASK_USER_SS(%si),%di // entry SS = current->t_regs.ss?
	je	utask1		// User using the right stack
//
//	System got crazy
//
	mov	$pmsg,%ax
	push	%ax
	call	panic
utask1:
#endif
//
//	Switch to kernel stack
//
	add	$TASK_USER_DI,%si
//
//	Save segment, index, BP and SP registers
//
save_regs:
	incw	_gint_count
	pop	(%si)		// DI
	pop	2(%si)		// SI
	pop	8(%si)		// DS
	pop	%di		// Pointer to interrupt number
	push	%bp		// BP
	mov	%sp,10(%si)	// SP
	mov	%ss,12(%si)	// SS
	mov	%es,6(%si)	// ES
	mov	%ax,4(%si)	// orig_ax
//
//	Load new segment and SP registers
//
	mov	%si,%sp
	mov	%ds,%si
	mov	%si,%ss
	mov	%si,%es
//
//	Save remaining registers
//
	push	%dx		// DX
	push	%cx		// CX
	push	%bx		// BX
	push	%ax		// AX
//
//	cs:[di] has interrupt number
//
//	seg	cs
	movb	%cs:(%di),%al
	cmpb	$0x80,%al
	jne	updct
//
//	----------PROCESS SYSCALL----------
//
	sti
	call	stack_check	// Check USER stack
	pop	%ax		// Get syscall function code
#ifdef CONFIG_STRACE
//
//	strace(syscall#, params...)
//
	push	%ax
	call	strace		// strace must be compiled with tail function optim off
	pop     %ax
#endif
//
//	syscall(params...)
//
	call	syscall
	push	%ax		// syscall returns a value in ax
#ifdef CONFIG_STRACE
//
//	ret_strace(retval)
//
	call	ret_strace
#endif
//
//	Restore registers
//
	call	do_signal
	cli
	jmp	restore_regs
//
//	Done.
//

//
// Called by run_init_process after sys_execve for the init task (/bin/init).
// Stack setup by kfork_proc(init_task) and arch_build_stack.
// Switch to kernel stack specified by 'current' and return ax=0 into user mode.
//
ret_from_syscall:
	mov	current,%bx	// Ensure we have the
	lea	TASK_USER_BX(%bx),%sp // right kernel SP
	xor	%ax,%ax		// Just in case we are starting a new task
	push	%ax
	cli
	jmp	restore_regs
/*
!
!	----------PROCESS INTERRUPT----------
!
!	Update intr_count
!
*/
updct:
	incw	intr_count	// only needed for schedule during interrupt warning
//
//	Call the C code
//
	sti			// Reenable interrupts
	mov	%sp,%bx		// Get pointer to pt_regs
	cbw
	push	%ax		// IRQ for later

	push	%bx		// Register base
	push	%ax		// IRQ number
	call	do_IRQ		// Do the work
	pop	%ax		// Clean parameters
	pop	%bx

	pop	%ax		// Saved IRQ
//
//	Send EOI to interrupt controller
//
	cli			// Disable interrupts to avoid reentering ISR
	cmp	$16,%ax
	jge	was_trap	// Traps need no reset
	or	%ax,%ax		// Is int #0?
	jnz	a4
//
//	IRQ 0 (timer) has to go on to the bios for some systems
//
	decw	_bios_call_cnt_l // Will call bios int?
	jne	a4
	movw	$5,_bios_call_cnt_l
	pushf
	lcall	*_stashed_irq0_l
	jmp	was_trap	// EOI already sent by bios int
a4:
	cmp	$8,%ax
	mov	$0x20,%al	// EOI
	jb	a6		// IRQ on low chip
/*
!
!	Reset secondary 8259 if we have taken an AT rather
!	than XT irq. We also have to prod the primay
!	controller EOI..
!
*/
	out	%al,$0xA0
	jmp	a5
a5:	jmp	a6
a6:	out	%al,$0x20		// Ack on primary controller
//
//	And a trap does no hardware work
//
was_trap:
//
//	Restore intr_count
//
	decw	intr_count
//
//	Now look at rescheduling
//
	cmpw	$1,_gint_count
	jne	restore_regs	// No
//	cmp	$0,_need_resched // Schedule needed ?
//	je	restore_regs	// No
//
// This path will return directly to user space
//
	sti			// Enable interrupts to help fast devices
	call	schedule	// Task switch
	call	do_signal	// Check signals
	cli
//
//	Restore registers and return
//
restore_regs:
	decw	_gint_count
	pop	%ax
	pop	%bx
	pop	%cx
	pop	%dx
	pop	%di
	pop	%si
	pop	%bp		// discard orig_AX
	pop	%es
	pop	%ds
	pop	%bp		// SP
	pop	%ss
	mov	%bp,%sp
	pop	%bp		// user BP
//
//	Iret restores CS:IP and F (thus including the interrupt bit)
//
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

	.global	tswitch

tswitch:
	push	%bp		// schedule()'s bp
#ifdef USE_IA16
	push	%es
#endif
	push	%di
	push	%si
	mov	previous,%bx
	mov	%sp,TASK_KRNL_SP(%bx)
	mov	current,%bx
	mov	TASK_KRNL_SP(%bx),%sp
	pop	%si
	pop	%di
#ifdef USE_IA16
	pop	%es
#endif
	pop	%bp		// BP of schedule()
	ret


//; Halt on idle helper
//; Wait for next interrupt to save CPU power

#ifdef CONFIG_IDLE_HALT

	.global idle_halt

idle_halt:
	hlt
	ret

#endif


	.data
	.global	intr_count
	.extern	current
	.extern	previous
	.extern	_kernel_ds

	.align 1

_bios_call_cnt_l:
	.word	5
_stashed_irq0_l:
	.long	0
intr_count:			// Hardware interrupts count
	.word	0
_gint_count:			// General interrupts count. Start with 1
	.word	1		// because init_task() is in kernel mode
#ifdef CHECK_SS
pmsg:	.ascii "Running unknown code"
	.byte	0
#endif
	.skip 512,0		// (was) 128 byte interrupt stack
_intstack:

