#include <linuxmt/config.h>
#include <linuxmt/debug.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/signal.h>
#include <linuxmt/types.h>

#include <arch/segment.h>
#include <arch/asm-offsets.h>

#ifdef CONFIG_ROMCODE

#define stashed_ds	[0]

#else

#ifndef S_SPLINT_S
#asm

    .text
  
/*
 *  This code is either in code segment or CONFIG_ROM_IRQ_DATA 
 *  The CS-Code must always be placed in irqtab.c, because the
 *  linker doesnt store them in block.
 */

    .extern stashed_ds

/* and now code */

#endasm
#endif

#endif

extern int do_signal(void);

void sig_check(void)
{
    if (current->signal)
        do_signal();
}

/*
 *	tswitch();
 *
 *	This function can only be called with SS=DS=ES=kernel DS and
 *	CS=kernel CS. SS:SP is the relevant kernel stack (IRQ's are
 *	taken on 'current' kernel stack. Thus we don't need to arse about
 *	with segment registers. The kernel isn't relocating.
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
 *	tswitch() saves the "previous" task registers and state. It in effect
 *	freezes a copy of the caller context. Then restores the "current"
 *	context and returns running the current task.
 *
 * ELKS 0.76 7/1999  Fixed for ROMCODE-Version
 * Christian Mardm”ller  (chm@kdt.de)
 */
#ifndef S_SPLINT_S
#asm
	.text

	.globl _tswitch

_tswitch:
	push bp		! /* schedule()'s bp */
	pushf
	push di
	push si
	mov bx,_previous
	mov TASK_KRNL_SP[bx],sp
	mov bx,_current
	mov sp,TASK_KRNL_SP[bx]
	pop si
	pop di
	popf
	pop bp		! BP of schedule()
	xor ax,ax	! Set ax=0, as this may be fork() return from child
	ret		! thus to caller of schedule()

!
!	System Call Vector
!
!	On entry we are on the wrong stack, DS, ES are wrong
!

	.globl _syscall_int

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
        push si
!
!	Load kernel data segment
!
#ifdef CONFIG_ROMCODE
        mov si,#CONFIG_ROM_IRQ_DATA
        mov ds,si
#else
        seg cs
#endif        
	mov ds,stashed_ds	! the org DS of kernel
!
!	At this point, the kernel stack is empty. Thus, we can push
!       data into the kernel stack by writing directly to memory
!
	mov si,_current         ! pops SI from user stack and pushes
	pop TASK_KSTKT_SI[si]   ! it directly into kernel stack
!
!	Stash user mode stack - needed for stack checking!
!
	mov TASK_USER_SP[si],sp
!
!	load kernel stack pointer
!
        lea sp,TASK_KSTKT_SI[si]
!
!	Finish switching to the right things
!
	mov si,ds	! ds=es=ss
	mov es,si
	mov ss,si
	cld
!
!	Stack is now right, we can take interrupts OK
!
	sti             ! SI already on top of stack
        push    di
	push	dx
	push	cx
	push	bx

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
        mov     bx,_current
        mov     8[bx],#0
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
	mov	bx,_current
!
!	At this point, the kernel stack is empty. Thus, there is no
!       need to save the kernel stack pointer.
!
	mov 	sp,TASK_USER_SP[bx]
	mov	bx,TASK_USER_SS[bx]
!
!	User segment recovery
!
	mov	ds,bx
	mov	es,bx
	mov	ss,bx
!
!	return with error info.
!
	iret
!
!	Done.
!
#endasm
#endif

int run_init_process(char *cmd, char *ar)
{
    int num;
    unsigned short int *pip = (unsigned short int *)ar;

    *pip++ = 0;
    *pip++ = (unsigned short int) &ar[6];
    *pip++ = 0;
    if(num = sys_execve(cmd, ar, 18)) {
	printk("sys_execve(\"%s\", args, 18) => %d.\n", cmd, -num);
	return num;
    }
#ifndef S_SPLINT_S
    /* Brackets round the following code are required as a work around
     * for a bug in the compiler which causes it to jump past the asm
     * code if they are not there.
     */
    {
#asm
        br  _ret_from_syscall
#endasm
    }
#endif
}

/*
 * We only need to do this as long as we support old format binaries
 * that grow stack and heap towards each other
 */
void stack_check(void)
{
    register __ptask currentp = current;
/*    register segext_t end;*/ /* Unused variable "end" */

    if ((currentp->t_begstack > currentp->t_enddata) &&
	(currentp->t_regs.sp < currentp->t_endbrk)) {
/*	end = currentp->t_endbrk;*/
	goto stack_overflow;
    }
#ifdef CONFIG_EXEC_ELKS
    else if(currentp->t_regs.sp > currentp->t_endseg) {
/*        end = 0xffff;*/
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

void kfork_proc(char *addr)
{
    register struct task_struct *t;

    t = find_empty_process();

    t->t_regs.cs = get_cs();
    t->t_regs.ds = t->t_regs.ss = get_ds(); /* Run in kernel space */
    arch_build_stack(t, addr);
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
    register char *i;

    i = 0;
    if(t->t_regs.bx != 0) {
        for(; (int)i < 18; i += 2)
            put_ustack(t, (int)i-4, (int)get_ustack(t,(int)i));
    }
    debug4("Stack %x was %x %x %x\n", addr, get_ustack(t,(int)i), get_ustack(t,(int)i+2),
	   get_ustack(t,(int)i+4));
    put_ustack(t, (int)i-4, (int)addr);
    put_ustack(t, (int)i-2, (int)get_ustack(t,(int)i+2));
    put_ustack(t, (int)i+2, (int)get_ustack(t,(int)i));
    put_ustack(t, (int)i, (int)get_ustack(t,(int)i+4));
    put_ustack(t, (int)i+4, (int)signr);
    t->t_regs.sp -= 4;
    debug5("Stack is %x %x %x %x %x\n", get_ustack(t,(int)i), get_ustack(t,(int)i+2),
	   get_ustack(t,(int)i+4),get_ustack(t,(int)i+6), get_ustack(t,(int)i+8));
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

/*
 *	arch_build_stack(t, addr);
 *
 * Build a fake return stack in kernel space so that
 * we can have a new task start at a chosen kernel
 * function while on its kernel stack. We push the
 * registers suitably for
 */

extern void ret_from_syscall();		/* our return address */

void arch_build_stack(struct task_struct *t, char *addr)
{
    register __u16 *tsp = (__u16 *)(t->t_kstack + KSTACK_BYTES - 10);
    register __u16 *csp = (__u16 *)(current->t_kstack + KSTACK_BYTES - 14);

    t->t_regs.ksp = (__u16)tsp;
    *tsp++ = *(csp + 6);	/* Initial value for SI register */
    *tsp++ = *(csp + 5);	/* Initial value for DI register */
    *tsp++ = 0x3202;		/* Initial value for FLAGS register */
    *tsp++ = *csp;		/* Initial value for BP register */
    if(addr == NULL)
	addr = ret_from_syscall;
    *tsp = addr;		/* Start execution address */
}
