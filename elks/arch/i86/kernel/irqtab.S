#include <linuxmt/config.h>
#include <arch/asm-offsets.h>
#include <arch/ports.h>

#define CHECK_SS	// integrity check interrupt from user mode

	.arch	i8086, nojumps
	.code16
	.text

/*
 *	Easy way to find the kernel DS
 *
 * Define a variable in the code segment whose
 * predefined contents is the kernel data segment
 */

	.align 1

	.global	irqtab_init
irqtab_init:

	xor	%ax,%ax
	mov	%ax,%ds

// CS points to this kernel code segment
// DS points to page 0  (interrupt table)
// ES points to the kernel data segment

	mov	$32,%bx
	mov	(%bx),%ax		// get the old timer intr
//	seg	es
	mov	%ax,%es:_stashed_irq0_l
	mov	2(%bx),%ax
//	seg	es
	mov	%ax,%es:_stashed_irq0_l+2

//
// Tidy up
//
	push	%ss		// restore DS
	pop	%ds
	ret

//------------------------------------------------------------------------------
// Set interrupt vector
//------------------------------------------------------------------------------

	.global int_vector_set

// void_vector_set (int vect, int_proc proc, int seg);
// arg1: vector number (byte pushed as word by the C compiler)
// arg2: function offset (word)
// arg3: function segment (word)

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
	.extern	stack_check
	.extern	syscall
#ifdef CHECK_SS
	.extern	panic
#endif
#ifdef CONFIG_STRACE
	.extern	strace
	.extern	ret_strace
#endif

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
	//mov	%sp,%si
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


// Halt on idle helper
// Wait for next interrupt to save CPU power

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

