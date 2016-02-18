#include <linuxmt/config.h>
#include <linuxmt/debug.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/signal.h>
#include <linuxmt/types.h>

#include <arch/segment.h>

static char *args[] = {
    0x01,	/* argc     */
    0x08,	/* &argv[0] */
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
    if(!(num = sys_execve(cmd, args, 20))) {
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

    if(currentp->t_begstack > currentp->t_enddata) {
	if(currentp->t_regs.sp > currentp->t_endbrk)
	    return;
    }
    else if(currentp->t_regs.sp < currentp->t_endseg)
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

    t->t_regs.cs = kernel_cs;
    t->t_regs.ds = t->t_regs.ss = kernel_ds; /* Run in kernel space */
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
 */
void arch_setup_kernel_stack(register struct task_struct *t)
{
    put_ustack(t, -2, USER_FLAGS);			/* Flags */
    put_ustack(t, -4, (int) current->t_regs.cs);	/* user CS */
    put_ustack(t, -6, 0);				/* addr 0 */
    t->t_regs.sp -= 6;
    t->t_kstackm = KSTACK_MAGIC;
}

/* Called by do_signal()
 *
 * We need to make the program return to another point - to the signal
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
 * To start a child process we need to craft for it a kernel stack. The
 * child user stack must be the same than the caller user stack. The stack
 * state inside do_fork for the CALLER of sys_fork() looks like this:
 *
 *             Kernel Stack                               User Stack
 *     ?? ip bx cx dx di si                                  ip cs f
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
 *           Kernel Stack                               User Stack
 *          si di f bp IP                                  ip cs f
 *
 * with IP pointing to ret_from_syscall, and current->t_regs.ksp pointing
 * to si on the kernel stack. Values for the child stack si, di are taken
 * from the caller's stack and bp is taken from caller->t_regs.bp.
 *
 */

/*
 *	arch_build_stack(t, addr);
 *
 * Called by do_fork() and kfork_proc().
 *
 * Build a fake return stack in kernel space so that we can
 * have a new task start at a chosen kernel function while on
 * its kernel stack. The new task comes into life in the middle
 * of tswitch() function. We push the registers suitably for
 * the new task to return from tswitch() into the function
 * addr().
 */

void arch_build_stack(struct task_struct *t, char *addr)
{
    register __u16 *tsp = (__u16 *)(t->t_kstack + KSTACK_BYTES - 10);
    register __u16 *csp = (__u16 *)(current->t_kstack + KSTACK_BYTES - 14);

    t->t_regs.ksp = (__u16)tsp;
    *tsp++ = *(csp + 6);	/* Initial value for SI register */
    *tsp++ = *(csp + 5);	/* Initial value for DI register */
    *tsp++ = 0x3202;		/* Initial value for FLAGS register */
    *tsp++ = current->t_regs.bp;/* Initial value for BP register */
    if(addr == NULL)
	addr = ret_from_syscall;
    *tsp = addr;		/* Start execution address */
}
