#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>

#ifdef __BCC__
#define offsetof(s,m) (size_t)&(((s *)0)->m)
#else
#ifdef __WATCOMC__
#define offsetof(__typ,__id) ((size_t)((char *)&(((__typ*)0)->__id) - (char *)0))
#else
#include <stddef.h>
#endif
#endif

extern int TASK_KRNL_SP, TASK_USER_SP, TASK_USER_SS;
extern int TASK_KSTKTOP, TASK_KSTKT_SI;

void asm_offsets(void)
{
    TASK_KRNL_SP = offsetof(struct task_struct, t_regs.ksp);
    TASK_USER_SP = offsetof(struct task_struct, t_regs.sp);
    TASK_USER_SS = offsetof(struct task_struct, t_regs.ss);
    TASK_KSTKTOP = offsetof(struct task_struct, t_kstack) + KSTACK_BYTES;
    TASK_KSTKT_SI = offsetof(struct task_struct, t_kstack) + KSTACK_BYTES - 2;
}

