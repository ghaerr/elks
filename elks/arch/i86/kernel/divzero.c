#include <linuxmt/config.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>

/*
 * Divide by zero and divide overflow exception handler
 *
 * 19 Aug 24 Greg Haerr
 */

#define DEBUG       0   /* =1 for CS:IP detail of fault */

void div0_handler(int irq, struct pt_regs *regs)
{
    if (intr_count > 1 /*|| current->t_regs.ss == kernel_ds*/) {
        /*
         * Trap from kernel code or idle task.
         *
         * Not calling panic at this point involves determining the CS:IP of the
         * faulting instruction, then doing two different things depending on
         * whether the CPU is 8088/8086/V20/V30 or 286/386+: the former trap
         * pushes the CS:IP following the DIV, while the latter pushes CS:IP of
         * the DIV instruction. Incrementing IP on 286+ would involve decoding
         * all forms of DIV, while allowing either would introduce undefined
         * behaviour as a result of an incorrect calcuation. So we panic.
         */

#if DEBUG
        struct uregs __far *sys_stack;
        sys_stack = _MK_FP(regs->ss, regs->sp);
        printk("CS:IP %04x:%04x\n", sys_stack->cs, sys_stack->ip);
#endif

        panic("DIVIDE FAULT\n");
    } else {    /* For user mode faults, display error message and kill the process */
        printk("DIVIDE FAULT\n");

#if DEBUG
        struct uregs __far *user_stack;
        user_stack = _MK_FP(current->t_regs.ss, current->t_regs.sp);
        printk("CS:IP %04x:%04x\n", user_stack->cs, user_stack->ip);
#endif

        sys_kill(current->pid, SIGABRT);    /* no SIGFPE so send SIGABRT for now */
    }
}
