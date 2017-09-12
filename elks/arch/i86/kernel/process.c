#include <linuxmt/config.h>
#include <linuxmt/debug.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/signal.h>
#include <linuxmt/types.h>

#include <arch/segment.h>

static char *args[] = {
(char *)0x01,	/* argc     */
(char *)0x08,	/* &argv[0] */
    NULL,	/* end argv */
    NULL,	/* envp     */
    NULL,	/* argv[0]     */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

extern void ret_from_syscall(void);

int run_init_process(register char *cmd)
{
    int num;

    strcpy((char *)(&args[4]), cmd);
    if (!(num = sys_execve(cmd, args, 20))) {
	ret_from_syscall();
    }
    printk("sys_execve(\"%s\", args, 18) => %d.\n", cmd, -num);
    return num;
}

/*
 * We only need to do this as long as we support old format binaries
 * that grow stack and heap towards each other
 */
void stack_check(void)
{
    register __ptask currentp = current;
/*    register segext_t end;*/ /* Unused variable "end" */

    if (currentp->t_begstack > currentp->t_enddata) {
	if (currentp->t_regs.sp > currentp->t_endbrk)
	    return;
    }
    else if (currentp->t_regs.sp < currentp->t_endseg)
	return;
    printk("STACK OVERFLOW BY %u BYTES\n", 0xffff - currentp->t_regs.sp);
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

    t->t_xregs.cs = kernel_cs;			/* Run in kernel space */
    t->t_regs.ds = t->t_regs.es = t->t_regs.ss = kernel_ds;
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

/*
 * Called by sys_execve()
 * Despite its name, this function sets up the user stack
 */
void arch_setup_kernel_stack(register struct task_struct *t)
{
    put_ustack(t, -2, USER_FLAGS);		/* Flags */
    put_ustack(t, -4, (int) t->t_xregs.cs);	/* user CS */
    put_ustack(t, -6, 0);			/* addr 0 */
    put_ustack(t, -8, 0);			/* user BP */
    t->t_regs.sp -= 8;
}

/* Called by do_signal()
 *
 * We need to make the program return to another point - to the signal
 * handler. The user stack currently looks like this:-
 *
 *          bp  ip  cs  f
 *
 * and will look like this
 *
 *  bp  adr cs  f   ip  sig
 *
 * so that we return to the old point afterwards. This will confuse the code
 * as we don't have any way of sorting out a return value yet.
 */

void arch_setup_sighandler_stack(register struct task_struct *t,
				 __sighandler_t addr,unsigned signr)
{
    debug5("Stack %x was %x %x %x %x\n", addr, get_ustack(t,0), get_ustack(t,2),
	   get_ustack(t,4), get_ustack(t,6));
    put_ustack(t, -4, (int)get_ustack(t,0));
    put_ustack(t, -2, (int)addr);
    put_ustack(t, 0, (int)get_ustack(t,4));
    put_ustack(t, 4, (int)get_ustack(t,2));
    put_ustack(t, 2, (int)get_ustack(t,6));
    put_ustack(t, 6, (int)signr);
    t->t_regs.sp -= 4;
    debug6("Stack is %x %x %x %x %x %x\n", get_ustack(t,0), get_ustack(t,2),
	   get_ustack(t,4), get_ustack(t,6), get_ustack(t,8), get_ustack(t,10));
}

/*
 *	arch_build_stack(t, addr);
 *
 * Called by do_fork() and kfork_proc().
 *
 * Build a fake return stack in kernel space so that we can have a new
 * task start at a chosen kernel function while on its kernel stack. The
 * new task comes into life in the middle of tswitch() function. We push
 * the registers suitably for the new task to return from tswitch() into
 * the function addr().
 *
 * To start a child process we need to craft for it a kernel stack. The
 * child user stack must be the same than the caller user stack. The stack
 * state inside do_fork for the CALLER of sys_fork() looks like this:
 *
 *             Kernel Stack                              User Stack
 *     ?? ip bx cx dx di si                              bp ip cs f
 *           --------------
 *           syscall params
 *
 * The user ss(=ds,es) and sp are stored in current->t_regs by the
 * int code, before changing to kernel stack
 *
 * Our child process needs to look as if it had called tswitch(), and
 * when it goes back into userland give ax = 0. This is conveniently
 * achieved by making tswitch() return ax=0. Stack for the CHILD must look
 * like this:
 *
 *             Kernel Stack                              User Stack
 *              si di bp IP: BCC case                    bp ip cs f
 *           si di es bp IP: IA16-GCC case               bp ip cs f
 *
 * with IP pointing to ret_from_syscall, and current->t_xregs.ksp pointing
 * to si on the kernel stack. Values for the child stack si, di and bp can
 * be anything because their final value will be taken from the task structure
 * in the case of fork(), or will be initialized at the begining of the target
 * function in the case of kfork_proc().
 */

void arch_build_stack(struct task_struct *t, char *addr)
{
    register __u16 *tsp = ((__u16 *)(&(t->t_regs.ax))) - 1;

    if (addr == NULL)
	addr = (char *)ret_from_syscall;
    *tsp = (__u16 *)addr;		/* Start execution address */
#ifdef __ia16__
    *(tsp-2) = kernel_ds;		/* Initial value for ES register */
    t->t_xregs.ksp = (__u16)(tsp - 4);	/* Initial value for SP register */
#else
    t->t_xregs.ksp = (__u16)(tsp - 3);	/* Initial value for SP register */
#endif
}
