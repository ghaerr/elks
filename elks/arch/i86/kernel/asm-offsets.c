#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>

extern int TASK_KRNL_SP, TASK_USER_DS, TASK_USER_AX, TASK_USER_SS;
extern int TASK_USER_BX, TASK_USER_SI, TASK_USER_DI;

void asm_offsets(void)
{
    TASK_KRNL_SP = offsetof(struct task_struct, t_xregs.ksp);
    TASK_USER_DS = offsetof(struct task_struct, t_regs.ds);
    TASK_USER_AX = offsetof(struct task_struct, t_regs.ax);
    TASK_USER_SS = offsetof(struct task_struct, t_regs.ss);
    TASK_USER_BX = offsetof(struct task_struct, t_regs.bx);
    TASK_USER_SI = offsetof(struct task_struct, t_regs.si);
    TASK_USER_DI = offsetof(struct task_struct, t_regs.di);
}

