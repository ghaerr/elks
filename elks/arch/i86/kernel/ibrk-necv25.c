#include <linuxmt/config.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>

/*
 * IBRK handler - NEC V25 specific
 * 
 * For ELKS the IBRK flag in the processor status word should always be set to 1.
 * This means: No IO emulation mode. But the IBRK could be reset to 0 by a 
 * misbehaving program. Then, when an IN or OUT assembler instruction is 
 * about to excecute, an INT-13 interrupt is activated. Setting
 * the IBRK here re-excecutes the IO instruction on return from interrupt.
 */

void ibrk_handler(int irq, struct pt_regs *regs)
{
    if (intr_count > 1 /*|| current->t_regs.ss == kernel_ds*/) {
        struct uregs __far *sys_stack;
        sys_stack = _MK_FP(regs->ss, regs->sp);
#if IBRK_VERBOSE_MODE
        printk("IBRK EXCEPTION AT CS:IP %04x:%04x PSW %04x\n", sys_stack->cs, sys_stack->ip, sys_stack->f);
#endif
        sys_stack->f |=0x02;    // set IBRK flag for restart of IO instruction
    } else {    /* For user mode faults, display error message and kill the process */
        struct uregs __far *user_stack;
        user_stack = _MK_FP(current->t_regs.ss, current->t_regs.sp);
#if IBRK_VERBOSE_MODE
        printk("IBRK EXCEPTION AT CS:IP %04x:%04x PSW %04x\n", user_stack->cs, user_stack->ip, user_stack->f);
#endif
        user_stack->f |=0x02;    // set IBRK flag for restart of IO instruction
    }
}
