#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/config.h>

/*
 *	This function can only be called with SS=DS=ES=kernel DS
 *	CS=kernel CS. SS:SP is the relevant kernel stack (IRQ's are
 *	take on 'current' kernel stack.
 *
 *	load_regs can also only be called from such a situation, thus
 *	we don't need to arse about with segment registers. The kernel isnt
 *	relocating.
 *
 *	To understand this you need to know how the compilers generate 8086
 *	stack frames. Functions normally start
 *
 *	push bp		! Save callers BP
 *	mov  bp,sp	! BP so we can use it to index registers
 *
 *	and end
 *
 *	mov sp,bp	! Fastest way to destroy local variables
 *	pop bp		! Restore callers BP
 *	ret		! Return address is top of stack now
 *
 *	save_regs() saves the callers registers and state then returns to
 *	the caller. It in effect freezes a copy of the caller context but doesnt
 *	prevent it being used temporarily beyond that as we do. 
 *
 *	load_regs() restores the callers context and returns skipping out of 
 *	schedule() [our the faked setup] back to the right place. 
 *
 *	fake_save_regs builds a stack frame that returns a new task to a
 *	kernel address of our choice using its own stack/context.
 *
 */

#asm
	.text
	.even
	.globl _save_regs
	
_save_regs:
!
!	Save the CPU registers (note: only need those which bcc would
!	expect to be preserved through a function call)
!	
	pop ax		! Return address save
	push bp		! /* schedule()'s bp */
	pushf
	push di
	push si
	push bx
	push dx
!
!	Store KSP
!
	mov  bx,_current
	mov  [bx],sp
	push ax		! Return with the stack altered.
	ret

! /*
!	Unlike save_regs this doesn't return to the caller but
!	to the callers caller.
! */
	.globl _load_regs

_load_regs:
!
!	Recover KSP
!	
	mov bx,_current
	mov ax,[bx]
	mov sp,ax
!
!	Pull CPU state off the stack
!
	pop dx
	pop bx
	pop si
	pop di
	popf
!
!	Now we basically spoof the return code of schedule()
!
	pop bp		! BP of schedule()
	mov sp,bp	! As schedule() would do on its return to get SP
	pop bp		! Recover caller BP
	xor ax,ax	! /* Set ax=0, as this may be child's fork() return */
	ret		! thus to caller of schedule()
	
!
!	System Call Vector
!
!	On entry we are on the wrong stack, DS, ES are wrong
!

	.globl _syscall_int
! This is from irqtab.c
	.extern stashed_ds
stashed_si:
        .word 0

!
!	System calls enter here with ax as function and bx,cx,dx as
!	parameters (and di,si if elks_syscall in elksemu is to be believed)
!	syscall returns a value in ax
!
_syscall_int:
!
!	We know the process DS, we can discard it (indeed may change it)
!
	cli
	seg cs
	mov sc_tmp,ax
	seg cs
	mov ax,stashed_ds
	mov ds,ax
! Save si
        seg cs
        mov stashed_si, si
! Free an index register	
	mov si,bx
!
!	Find our TCB
!
	mov bx,_current
!
!	Stash user mode stack - needed for stack checking!
!
	mov ax,sp
	mov 2[bx],ax
!
!	Finish switching to the right things
!
	mov ax,[bx]	! kernel stack pointer
	mov sp,ax
	mov ax,ds	! ds=es=ss
	mov es,ax
	mov ss,ax
	cld
!
!	Stack is now right, we can take interrupts OK
!
	sti
	seg     cs
	mov     ax, stashed_si
	push    ax              ! push si
        push    di
	push	dx
	push	cx
	push	si		! saved bx
	seg	cs
	mov	ax,sc_tmp	! restore ax
#ifdef CONFIG_STRACE
!
!	strace(syscall#, params...)
!
	push ax
	call _strace
	pop ax
#endif
!
!	syscall(params...)
!
	push ax
	call	_stack_check
	pop ax
	call	_syscall
	pop	bx
	pop	cx
	pop	dx
        pop     di
	pop     si
#ifdef CONFIG_STRACE
!
!	ret_strace(retval)
!
	push ax
	call _ret_strace
	pop ax
#endif
!
!	Now mend everything
!
_ret_from_syscall:
	cli
	mov	dx,ax
	mov	bx,_current
	mov	[bx],sp		! current->ksp is altered by schedule()
	mov 	sp,2[bx]
	mov	ax,4[bx]
!
!	User segment recovery
!
	mov	ds,ax
	mov	es,ax
	mov	ss,ax
!
!	return with error info.
!
	mov	ax,dx
	iret
!
!	Done.
!
	.even	
sc_tmp:	.word 0

	.globl _fake_save_regs
!
!	int used=fake_save_regs(sp,addr);
!
!	Build a fake return stack in kernel space so that
!	we can have a new task start at a chosen kernel 
!	function while on its kernel stack. We push the
!	registers suitably for
!
_fake_save_regs:
	push 	bp
	mov	bp,sp
	mov     bx,4[bp]	! new task ksp
	mov	ax,6[bp]	! new task start address
!
!	Build a dummy stack return frame
!
	mov	-2[bx],ax	! Return address
	mov	ax,[bp]		! Caller BP
	mov	-4[bx],ax	! Save caller BP
	mov	ax,bx		! Firstly get bx
	sub	ax,#4		! This is the apparent BP. It points
				! to caller BP and above it caller return
	mov	-6[bx],ax	! goes here.
!
!	Register State
!
	pushf
	pop	ax
	mov	-8[bx],ax	! Flags
	mov	-10[bx],di
	mov 	-12[bx],si
	mov	-14[bx],bx
	mov	-16[bx],dx
	mov	ax,#16
	pop	bp
	ret
#endasm

void stack_check()
{
	if (current->t_regs.sp < current->t_endbrk)
	{
		printk("STACK (%d) ENTERED BSS (%ld) - PROCESS TERMINATING\n", current->t_regs.sp, current->t_endbrk);
		sys_exit();
	}
}

/*
 *	Make task t fork into kernel space. We are in kernel mode
 *	so we fork onto our kernel stack.
 */
 
void kfork_proc(t, addr)
register struct task_struct *t;
char *addr;
{
	memset(t, 0, sizeof(struct task_struct));
	t->t_regs.ksp=t->t_kstack+KSTACK_BYTES;
	t->t_regs.ksp-=fake_save_regs(t->t_regs.ksp,addr);
	t->t_regs.ds=get_ds();
	t->state=TASK_UNINTERRUPTIBLE;
	t->pid=get_pid();
	t->t_priority=10;
	t->t_kstackm = KSTACK_MAGIC;
	t->prev_run = t->next_run = t->next_task = t->prev_task = NULL;
	wake_up_process(t);
	schedule();
}


/*
 *	Build a user return stack for exec*(). This is quite easy, especially
 *	as our syscall entry doesnt use the user stack. 
 */

#define USER_FLAGS 0x3200	/* IPL 3, interrupt enabled */
static void put_ustack(t, off, val)
register struct task_struct *t;
int off, val;
{
    pokew(t->t_regs.ss, t->t_regs.sp+off, val);
}

void arch_setup_kernel_stack(t)
register struct task_struct *t;
{
	put_ustack(t, -2, USER_FLAGS);		/* Flags */
	put_ustack(t, -4, current->t_regs.cs);	/* user CS */
	put_ustack(t, -6, 0);			/* addr 0 */
	t->t_regs.sp-=6;
	t->t_kstackm = KSTACK_MAGIC;
}

/*
 * There are two cases when a process could be switched out:
 *  (1) an interrupt occurred, and schedule() was called at the end.
 *  (2) a system call was made, and schedule() was called during it.
 *
 * In our case we are dealing with (2) since we the user called the fork()
 * system call. The stack state inside do_fork looks like this:
 *
 *                 Kernel Stack                               User Stack
 *               bp ip bx cx dx                                  ip cs f
 *                     --------
 *                     syscall params
 *
 * The user ss(=ds,es) and sp are stored in current->t_regs by the
 * int code, before changing to kernel stack
 *
 * Our child process needs to look as if we had called schedule(), and
 * when it goes back into userland give ax=0. This is conveniently
 * achieved by making load_regs return ax=0. Stack must look like this:
 *
 *                  Kernel Stack                               User Stack
 *       dx bx si di f bp bp IP                                  ip cs f
 *
 * with IP pointing to ret_from_syscall, sensible values for bp, and
 * current->t_regs.ksp pointing to dx on the kernel stack
 *
 * P.S. this is very similar to fake_save_regs, apart from the fiddling
 * we need to do to recover the user's bp.
 */

extern void ret_from_syscall();  /* our return address */

static void* saved_bp; /* we have to recover user's bp */

void arch_build_stack(t)
struct task_struct *t;
{
	char *kstktop = t->t_kstack+KSTACK_BYTES;
	t->t_regs.ksp=kstktop-fake_save_regs(kstktop,ret_from_syscall);
#asm
	mov bx,[bp]	! bx = bp on entry to arch_build_stack
	mov ax,[bx]	! bx = bp on entry to do_fork = users bp (hopefully!)
	mov _saved_bp,ax
#endasm
	*(void**)(kstktop-4) = saved_bp;
}

