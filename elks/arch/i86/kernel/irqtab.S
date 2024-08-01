#include <linuxmt/config.h>
#include <linuxmt/limits.h>
#include <linuxmt/trace.h>
#include <arch/asm-offsets.h>
#include <arch/ports.h>

	.arch	i8086, nojumps
	.code16
	.text

//------------------------------------------------------------------------------
// Save BIOS IRQ 0 timer vector
// void save_timer_irq(void)
//------------------------------------------------------------------------------

	.global	save_timer_irq
save_timer_irq:
	push	%es
	xor	%ax,%ax
	mov	%ax,%es		// ES -> interrupt table
	mov	$8*4,%bx	// INT 8 (IRQ 0) vector
	mov	%es:(%bx),%ax	// get the old timer intr
	mov	%ax,org_irq0
	mov	%es:2(%bx),%ax
	mov	%ax,org_irq0+2
	pop	%es
	ret

//------------------------------------------------------------------------------
// Set interrupt vector
//------------------------------------------------------------------------------
// void int_vector_set (int vect, int_proc proc, int seg);
// arg1: vector number (byte pushed as word by the C compiler)
// arg2: function offset (word)
// arg3: function segment (word)

	.global int_vector_set
int_vector_set:
	mov %sp,%bx
	mov 6(%bx),%dx  // arg3
	mov 4(%bx),%cx  // arg2
	mov 2(%bx),%bx  // arg1

	shl $1,%bx
	shl $1,%bx

	push %ds
	xor %ax,%ax
	mov %ax,%ds

	pushf
	cli
	mov %cx,0(%bx)
	mov %dx,2(%bx)
	popf

	pop %ds
	ret

//------------------------------------------------------------------------------
//
//	IRQ and IRQ return paths for Linux 8086
//
// The execution thread will not return from the function call.
// Instead, the address pushed in the stack will be used to get
// the interrupt number.

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
	.extern	syscall
	.extern	stack_check
	.extern	trace_begin
	.extern	trace_end
	.extern	panic

	.global _irqit
_irqit:
//
//	Make room
//
	push	%ds
	push	%si
	push	%di
//
//	Recover kernel data segment
//	Was pushed by the CALLF of the dynamic handler
//	TODO: BP is better for stack work
//
	mov	%sp,%si
	mov	%ss:8(%si),%ds
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
	sub	$8,%si		// 14 offsets less 6 already on stack
	jmp	save_regs
//
//	Using a process's kernel stack, switch to interrupt stack
//
itask:
	mov	$istack-14,%si  // 14 offsets 0-13 of SI below
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
	pop	%di		// Return offset is actually a pointer to the IRQ number
	pop	%ds		// Return segment of the dynamic handler = kernel DS
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
//	ds:[di] has IRQ number
//
	movb	(%di),%al
	cmpb	$0x80,%al
	jne	updct
//
//	----------PROCESS SYSCALL----------
//
	sti
	call	stack_check	// Check user mode stack

#ifdef CONFIG_TRACE
	call	trace_begin
#endif

	pop	%ax		// get syscall function code in AX
	call	syscall
	push	%ax		// syscall return value in ax

#ifdef CONFIG_TRACE
	// strace.c must be compiled with tail optimization off to protect top of stack
	call	trace_end       // syscall return value is top of stack
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
#ifdef CHECK_SCHED
	incw	intr_count	// only needed for schedule during interrupt warning
#endif
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
	cli			// Disable interrupts to avoid reentering ISR

#if defined(CONFIG_ARCH_IBMPC) || defined(CONFIG_ARCH_PC98)
//
//	Determine if trap or interrupt
//
	cmp	$16,%ax
	jge	was_trap	// Traps need no reset

#if defined(CONFIG_BLK_DEV_BFD) && !defined(CONFIG_ARCH_PC98)
	or	%ax,%ax		// Is int #0?
	jnz	do_eoi

//
//	IRQ 0 (timer) has to go on to the bios for some systems
//
	decw	bios_call_cnt	// Will call bios int?
	jne	do_eoi
	movw	$5,bios_call_cnt
	pushf
	lcall	*org_irq0
	jmp	was_trap	// EOI already sent by bios int
#endif

//
//	Send EOI to interrupt controller
//
do_eoi:
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
	out	%al,$PIC2_CMD		// Ack on secondary controller
	jmp	a5
a5:	jmp	a6
a6:	out	%al,$PIC1_CMD		// Ack on primary controller

#elif defined(CONFIG_ARCH_8018X)
//
//	Determine if trap or interrupt
//
	cmp	$16,%ax
	jge	was_trap	// Traps need no reset

	mov $0x8000, %ax // set the NSPEC bit on the
	mov $0xff02, %dx // EOI register so the ICU
	out %ax, %dx // acks the highest priority interrupt
#endif

//
//	And a trap does no hardware work
//
was_trap:
//
//	Restore intr_count
//
#ifdef CHECK_SCHED
	decw	intr_count
#endif
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
 *	tswitch()
 *
 *	This function can only be called with SS=DS=kernel DS and
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
	push	%es		// required for gcc-ia16
	push	%di
	push	%si
	mov	previous,%bx
	mov	%sp,TASK_KRNL_SP(%bx)
	mov	current,%bx
	mov	TASK_KRNL_SP(%bx),%sp
	pop	%si
	pop	%di
	pop	%es
	pop	%bp		// BP of schedule()
	ret

// setsp(void *sp) - set stack pointer
	.global setsp
setsp:
	pop	%bx		// return address
	pop	%ax
	mov	%ax,%sp
	jmp	*%bx

// Halt - wait for next interrupt to save CPU power
	.global idle_halt
idle_halt:
	hlt
	ret

	.data
	.global	intr_count
	.global	endistack
	.global	istack
	.extern	current
	.extern	previous

bios_call_cnt:                  // call BIOS IRQ 0 handler every 5th interrupt
	.word	5
org_irq0:			// original BIOS IRQ 0 vector
	.long	0
_gint_count:			// General interrupts count. Start with 1
	.word	1		// because init_task() is in kernel mode
#ifdef CHECK_SCHED
intr_count:			// Hardware interrupts count
	.word	0
#endif
#ifdef CHECK_SS
pmsg:	.ascii "Running unknown code"
	.byte	0
#endif
	.p2align 1
endistack:
	.skip ISTACK_BYTES,0	// interrupt stack
istack:
