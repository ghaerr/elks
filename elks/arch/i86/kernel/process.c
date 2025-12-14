#include <linuxmt/config.h>
#include <linuxmt/debug.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/string.h>
#include <linuxmt/sched.h>
#include <linuxmt/signal.h>
#include <linuxmt/types.h>
#include <linuxmt/memory.h>

#include <arch/segment.h>

static char *args[] = {
(char *)0x01,   /* 0 argc     */
(char *)0x08,   /* 2 &argv[0] */
    NULL,       /* 4 end argv */
    NULL,       /* 6 envp     */
    NULL,       /* 8-19 argv[0], 12 chars space for cmd path*/
    NULL,
    NULL,
    NULL,
    NULL,
    NULL        /* 18-19*/
};

int run_init_process(const char *cmd)
{
    int num;

    strcpy((char *)&args[4], cmd);
    if (!(num = sys_execve(cmd, (char *)args, sizeof(args))))
        ret_from_syscall();             /* no return, returns to user mode*/
    return num;
}

int run_init_process_sptr(const char *cmd, char *sptr, int slen)
{
    int num;

    if (!(num = sys_execve(cmd, sptr, slen)))
        ret_from_syscall();             /* no return, returns to user mode*/
    return num;
}

/*
 * Check that SP is within proper range, called before every syscall.
 */
void stack_check(void)
{
    segoff_t sp = current->t_regs.sp;
    segoff_t brk = current->t_endbrk;
    segoff_t stacklow = current->t_begstack - current->t_minstack;

    if (sp < brk) {
        printk("(%P)STACK OVERFLOW by %u\n", brk - sp);
        printk("curbreak %u, SP %u\n", current->t_endbrk, current->t_regs.sp);
        do_exit(SIGSEGV);
    }
    if (sp < stacklow) {
        /* notification only, allow process to continue */
        printk("(%P)STACK USING %u UNUSED HEAP\n", stacklow - sp);
    }
    if (sp > current->t_begstack) {
        printk("(%P)STACK UNDERFLOW\n");
        do_exit(SIGSEGV);
    }
}

/*
 *  Make task t fork into kernel space. We are in kernel mode
 *  so we fork onto our kernel stack.
 */

void kfork_proc(void (*addr)())
{
    struct task_struct *t;

    t = find_empty_process();

    /* All other t_regs values invalid for idle task or handlers interrupting idle task */
    t->t_regs.ds = t->t_regs.es = t->t_regs.ss = kernel_ds;
    if (addr)
        arch_build_stack(t, addr);
}

/*
 *  Build a user return stack for exec*(). This is quite easy,
 *  especially as our syscall entry doesnt use the user stack.
 */

#define USER_FLAGS 0xf200               /* IPL 3, interrupt enabled */

void put_ustack(register struct task_struct *t,int off,int val)
{
    pokew(t->t_regs.sp+off, t->t_regs.ss, (word_t) val);
}

unsigned get_ustack(register struct task_struct *t,int off)
{
    return peekw(t->t_regs.sp+off, t->t_regs.ss);
}

/*
 * Called by sys_execve()
 */
void arch_setup_user_stack (register struct task_struct * t, word_t entry, seg_t cseg)
{
    put_ustack(t, -2, USER_FLAGS);              /* Flags */
    put_ustack(t, -4, cseg);                    /* user CS */
    put_ustack(t, -6, entry);                   /* user entry point */
    put_ustack(t, -8, 0);                       /* user BP */
    t->t_regs.sp -= 8;
}

/* Called by do_signal()
 *
 * We need to make the program return to another point - to the signal
 * handler. The user stack currently looks like this:-
 *  [low address <---> high address ]
 *              bp  ip  cs  f
 *
 * and will look like this
 *
 *  bp  adr adr f   ip  cs  sig
 *       lo  hi
 *
 * so that we return to the old point afterwards. This will confuse the code
 * as we don't have any way of sorting out a return value yet.
 */

void arch_setup_sighandler_stack(register struct task_struct *t,
                                 __kern_sighandler_t addr,unsigned signr)
{
    debug("Stack %x:%x was %x %x %x %x\n", _FP_SEG(addr), _FP_OFF(addr),
           get_ustack(t,0), get_ustack(t,2), get_ustack(t,4), get_ustack(t,6));
    put_ustack(t, -6, (int)get_ustack(t,0));
    put_ustack(t, -4, _FP_OFF(addr));
    put_ustack(t, -2, _FP_SEG(addr));
    put_ustack(t, 0, (int)get_ustack(t,6));
    put_ustack(t, 6, (int)signr);
    t->t_regs.sp -= 6;
    debug("Stack is %x %x %x %x %x %x %x\n", get_ustack(t,0), get_ustack(t,2),
           get_ustack(t,4), get_ustack(t,6), get_ustack(t,8), get_ustack(t,10),
           get_ustack(t,12));
}

/*
 *  arch_build_stack(t, addr);
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
 *  [low address <---> high address ]
 *
 *             Kernel Stack                              User Stack
 *     ?? ip bx cx dx di si orig_ax es ds sp ss          bp ip cs f
 *           --------------
 *           syscall params
 *
 * The user ss(=ds,es) and sp are stored in current->t_regs by the
 * int code, before changing to kernel stack
 *
 * Our child process needs to look as if it had called tswitch(), and
 * when it goes back into userland give ax = 0. This is conveniently
 * achieved by making ret_from_syscall return ax=0. Stack for the CHILD must look
 * like this:
 *
 *             Kernel Stack                              User Stack
 *              si di bp IP: BCC case                    bp ip cs f
 *           si di es bp IP: IA16-GCC case               bp ip cs f
 *
 * with IP pointing to ret_from_syscall, and current->t_ksp pointing
 * to si on the kernel stack. Values for the child stack si, di and bp can
 * be anything because their final value will be taken from the task structure
 * in the case of fork(), or will be initialized at the beginning of the target
 * function in the case of kfork_proc().
 */

void arch_build_stack(struct task_struct *t, void (*addr)())
{
    register __u16 *tsp = ((__u16 *)(&(t->t_regs.ax))) - 1;

    if (addr == NULL)
        addr = ret_from_syscall;
    *tsp = (__u16)addr;                 /* Start execution address */
#ifdef __ia16__
    *(tsp-2) = kernel_ds;               /* Initial value for ES register */
    t->t_ksp = (__u16)(tsp - 4);        /* Initial value for SP register */
#else
    t->t_ksp = (__u16)(tsp - 3);        /* Initial value for SP register */
#endif
}

#if UNUSED
/*
 * Restart last system call.
 * Usage: instead of returning -ERESTARTSYS from kernel system call,
 * use "return restart_syscall()".
 */
int restart_syscall(void)
{
    struct uregs __far *user_stack;

    user_stack = _MK_FP(current->t_regs.ss, current->t_regs.sp);
    user_stack->ip -= 2;                /* backup to INT 80h*/
    return current->t_regs.orig_ax;     /* restore syscall # to user mode AX*/
}
#endif
