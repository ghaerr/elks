#include <linuxmt/config.h>
#include <linuxmt/debug.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/signal.h>
#include <linuxmt/types.h>

#include <arch/segment.h>

/*
 *	This function can only be called with SS=DS=ES=kernel DS
 *	CS=kernel CS. SS:SP is the relevant kernel stack (IRQ's are
 *	taken on 'current' kernel stack.
 *
 *	load_regs can also only be called from such a situation, thus
 *	we don't need to arse about with segment registers. The kernel isn't
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
 *	the caller. It in effect freezes a copy of the caller context but
 *	doesn't prevent it being used temporarily beyond that as we do. 
 *
 *	load_regs() restores the callers context and returns skipping out of 
 *	schedule() [our faked setup] back to the right place. 
 *
 *	fake_save_regs builds a stack frame that returns a new task to a
 *	kernel address of our choice using its own stack/context.
 *
 * ELKS 0.76 7/1999  Fixed for ROMCODE-Version
 * Christian Mardm”ller  (chm@kdt.de)
 */

#ifdef CONFIG_ROMCODE 

#define stashed_ds	[0]
#define stashed_si	[14]
#define sc_tmp		[16]

#else

#define stashed_ds	cseg_stashed_ds
#define stashed_si	cseg_stashed_si
#define sc_tmp		cseg_sc_tmp

#ifndef S_SPLINT_S
#asm

    .text
  
/*
 *  This code is either in code segment or CONFIG_ROM_IRQ_DATA 
 *  The CS-Code must always be placed in irqtab.c, because the
 *  linker doesnt store them in block.
 */

	.extern	cseg_stashed_ds
	.extern	cseg_stashed_si 	; Now in irqtab.c
	.extern	cseg_sc_tmp

/* and now code */

#endasm
#endif

#endif

typedef unsigned short int FsR;

extern int do_signal(void);
extern int fake_save_regs(FsR,FsR);

void sig_check(void)
{
    register __ptask currentp = current;

    if (currentp->signal)
	(void) do_signal();
    currentp->signal = 0;
}

#ifndef S_SPLINT_S
#asm
	.text

#if 1	

	.globl _tswitch

_tswitch:
	push bp		! /* schedule()'s bp */
	pushf
	push di
	push si
	push bx
	push dx
	mov bx,_previous
	mov [bx],sp
	mov bx,_current
	mov ax,[bx]
	mov sp,ax
	pop dx
	pop bx
	pop si
	pop di
	popf
	pop bp		! BP of schedule()
	xor ax,ax
	ret

#else

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
/*
 *	Unlike save_regs this does not return to the caller
 *	but to the caller's caller.
 */
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
	xor ax,ax	! Set ax=0, as this may be fork() return from child
	ret		! thus to caller of schedule()

#endif
	
!
!	System Call Vector
!
!	On entry we are on the wrong stack, DS, ES are wrong
!

	.globl _syscall_int

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

        push ax

#ifdef CONFIG_ROMCODE
        mov ax,#CONFIG_ROM_IRQ_DATA
#else
        mov ax,cs
#endif        

        mov ds,ax
        pop ax

	mov sc_tmp,ax
!
!	Save si and free an index register
!
	mov stashed_si, si
	mov si,bx

	mov ax,stashed_ds
	mov ds,ax            ;the org DS of kernel

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
#ifdef CONFIG_ROMCODE
        mov ax,#CONFIG_ROM_IRQ_DATA
#else
        mov ax,cs
#endif        
        mov ds,ax
        
	mov     ax, stashed_si
	push    ax              ! push si
        push    di
	push	dx
	push	cx
	push	si		! saved bx
	mov	ax,sc_tmp	! restore ax
        push es
        pop  ds            ;orig kernel ds

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
	push	ax
	call	_stack_check
	pop	ax
	call	_syscall
	push	ax
	call	_sig_check
	pop	ax
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
#if 0
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
#else
	push	bp
	mov	bp,sp
	mov     bx,4[bp]	! new task ksp
	mov	ax,6[bp]	! new task start address

	mov	-2[bx],ax	! Return address
	mov	ax,[bp]		! Caller BP
	mov	-4[bx],ax	! Save caller BP
	pushf
	pop	ax
	mov	-6[bx],ax	! Flags
	mov	-8[bx],di
	mov 	-10[bx],si
	mov	-12[bx],bx
	mov	-14[bx],dx
	mov	ax,#14
	pop	bp
	ret
#endif
#endasm
#endif

/*
 * We only need to do this as long as we support old format binaries
 * that grow stack and heap towards each other
 */
void stack_check(void)
{
    register __ptask currentp = current;
    register segext_t end;
    if ((currentp->t_begstack > currentp->t_enddata) &&
	(currentp->t_regs.sp < currentp->t_endbrk)) {
	end = currentp->t_endbrk;
	goto stack_overflow;
    }
#ifdef CONFIG_EXEC_ELKS 
    else if (currentp->t_regs.sp > currentp->t_endseg){
        end = 0xffff;
	goto stack_overflow; 
    }
#endif
    return;
stack_overflow:
    printk("STACK OVERFLOW BY %d BYTES\n", 0xffff - currentp->t_regs.sp);
    do_exit(SIGSEGV);
}

/*
 *	Make task t fork into kernel space. We are in kernel mode
 *	so we fork onto our kernel stack.
 */
 
void kfork_proc(register struct task_struct *t,char *addr)
{
    memset(t, 0, sizeof(struct task_struct));
    t->t_regs.ksp = ((__u16) t->t_kstack) + KSTACK_BYTES;
    t->t_regs.ksp -= fake_save_regs((FsR)t->t_regs.ksp,(FsR)addr);
    t->t_regs.ds = get_ds();
    t->state = TASK_UNINTERRUPTIBLE;
    t->pid = get_pid();

    t->prev_run = t->next_run = NULL;

    t->t_kstackm = KSTACK_MAGIC;
    wake_up_process(t);
    schedule();
}

/*
 *	Build a user return stack for exec*(). This is quite easy,
 *	especially as our syscall entry doesnt use the user stack. 
 */

#define USER_FLAGS 0x3200		/* IPL 3, interrupt enabled */

void put_ustack(register struct task_struct *t,int off,int val)
{
    pokew(t->t_regs.ss, t->t_regs.sp+off, (__u16) val);
}

unsigned get_ustack(register struct task_struct *t,int off)
{
    return peekw(t->t_regs.ss, t->t_regs.sp+off);
}

void arch_setup_kernel_stack(register struct task_struct *t)
{
    put_ustack(t, -2, USER_FLAGS);			/* Flags */
    put_ustack(t, -4, (int) current->t_regs.cs);	/* user CS */
    put_ustack(t, -6, 0);				/* addr 0 */
    t->t_regs.sp -= 6;
    t->t_kstackm = KSTACK_MAGIC;
}

/* We need to make the program return to another point - to the signal
 * handler. The stack currently looks like this:-
 *
 *              ip cs  f
 *
 * and will look like this
 *
 *      adr cs  f  ip  sig
 *
 * so that we return to the old point afterwards. This will confuse the code
 * as we don't have any way of sorting out a return value yet.
 */

void arch_setup_sighandler_stack(register struct task_struct *t,
				 __sighandler_t addr,unsigned signr)
{
    debug4("Stack %x was %x %x %x\n", addr, get_ustack(t,0), get_ustack(t,2),
	   get_ustack(t,4));
    put_ustack(t, 2, (int) get_ustack(t,0));
    put_ustack(t, 0, (int) get_ustack(t,4));
    put_ustack(t, 4, (int) signr);
    put_ustack(t, -2, (int)t->t_regs.cs);
    put_ustack(t, -4, (int) addr);
    t->t_regs.sp -= 4;
    debug5("Stack is %x %x %x %x %x\n", get_ustack(t,0), get_ustack(t,2),
	   get_ustack(t,4),get_ustack(t,6), get_ustack(t,8));
}

/*
 * There are two cases when a process could be switched out:
 *
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

extern void ret_from_syscall();		/* our return address */

static void* saved_bp;			/* we have to recover user's bp */

void arch_build_stack(struct task_struct *t)
{
    char *kstktop = (char *) t->t_kstack+KSTACK_BYTES;

/*@i3@*/ t->t_regs.ksp = kstktop - fake_save_regs(kstktop, ret_from_syscall);

{
#ifndef S_SPLINT_S
#asm
	mov	bx,[bp]	! bx = bp on entry to arch_build_stack
	mov	ax,[bx]	! ax = bp on entry to do_fork = users bp (hopefully!)
	mov	bx,ax
	mov	ax,[bx]	! ax = bp on entry to do_fork = users bp (hopefully!)
	mov	_saved_bp,ax
#endasm
#endif
}

    *(void**) (kstktop-4) = saved_bp;
}
