#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>

#ifdef __WATCOMC__
#define offsetof(__typ,__id) ((size_t)((char *)&(((__typ*)0)->__id) - (char *)0))
#else
#ifdef __ia16__
#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)
#else
#define offsetof(s,m) ((size_t)&(((s *)0)->m))
#endif
#endif

extern int TASK_KRNL_SP, TASK_USER_SP, TASK_USER_BP, TASK_USER_SS;
extern int TASK_USER_DS, TASK_KSTKTOP, TASK_KSTKT_SI;

void asm_offsets(void)
{
    TASK_KRNL_SP = offsetof(struct task_struct, t_regs.ksp);
    TASK_USER_SP = offsetof(struct task_struct, t_regs.sp);
    TASK_USER_BP = offsetof(struct task_struct, t_regs.bp);
    TASK_USER_SS = offsetof(struct task_struct, t_regs.ss);
    TASK_USER_DS = offsetof(struct task_struct, t_regs.ds);
    TASK_KSTKTOP = offsetof(struct task_struct, t_kstack) + KSTACK_BYTES;
    TASK_KSTKT_SI = offsetof(struct task_struct, t_kstack) + KSTACK_BYTES - 2;
}

