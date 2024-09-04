#include <linuxmt/config.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>

/*
 * Divide by zero and divide overflow exception handler
 *
 * 19 Aug 24 Greg Haerr
 */

static char div0msg[] = { "Divide fault\n" };

void div0_handler(int i, struct pt_regs *regs)
{
    /* divide by 0 from nested interrupt or idle task means kernel code was executing */
    if (_gint_count > 1 /*|| current->t_regs.ss == kernel_ds*/) {
        /*
         * Trap from kernel code.
         *
         * Not panicing at this point involves determining the CS:IP of the
         * faulting instruction, then doing two different things depending on
         * whether the CPU is 8088/8086/V20/V30 or 286/386+: the former trap
         * pushes the CS:IP following the DIV, while the latter pushes CS:IP of
         * the DIV instruction. Incrementing IP on 286+ would involve decoding
         * all forms of DIV, while allowing either would introduce undefined
         * behaviour as a result of an incorrect calcuation. So we panic.
         */
#if 0
        struct uregs __far *sys_stack;
        sys_stack = _MK_FP(regs->ss, regs->sp);
        printk("Div0 at CS:IP %x:%x\n", sys_stack->cs, sys_stack->ip);
#endif
        panic(div0msg);
    } else {
        /* For user mode faults, display error message and kill the process */
        printk(div0msg);
#if 0
        struct uregs __far *user_stack;
        user_stack = _MK_FP(current->t_regs.ss, current->t_regs.sp);
        printk("Div0 at CS:IP %x:%x\n", user_stack->cs, user_stack->ip);
#endif
        sys_kill(current->pid, SIGABRT);    /* no SIGFPE so send SIGABRT for now */
    }
}
