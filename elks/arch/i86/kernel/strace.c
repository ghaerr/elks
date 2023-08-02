#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/debug.h>
#include <linuxmt/config.h>
#include <linuxmt/wait.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
#include <linuxmt/string.h>

/* The table describing the system calls has been moved to a separate
 * header file, and is included by the following include line.
 */

#if defined(CONFIG_STRACE) || defined(CHECK_KSTACK)
#include "strace.h"

static struct sc_info *syscall_info(unsigned int callno)
{
    struct sc_info *s;
    static struct sc_info notimp = { "NOTIMP", 0};

    if (callno < sizeof(elks_table1)/sizeof(struct sc_info)) {
        s = &elks_table1[callno];
        if (s) return s;
    }
    else if (callno < sizeof(elks_table2)/sizeof(struct sc_info) + START_TABLE2) {
        s = &elks_table2[callno-START_TABLE2];
        if (s) return s;
    }
    return &notimp;
}

void check_kstack_init(void)
{
    memset(current->t_kstack, 0x55, KSTACK_BYTES-32);
}
#endif

#ifdef CONFIG_STRACE
static const char *fmtspec[] = {
    NULL,   "&0x%X", "0x%X",  "0x%X",
    "'%c'", "'%c'",  "\"%t\"","\"%t\"",
    "%u",   "%d",    "&%u",   "&%d",
    NULL,    NULL,    NULL,    NULL
};

struct sc_args {
    unsigned int args[3];    /* BX, CX, DX */
};

void strace(void)
{
    struct sc_info *s = syscall_info(current->t_regs.orig_ax);
    unsigned int info = (s->s_info >> 4);
    struct sc_args *p = (struct sc_args *)&current->t_regs.bx;
    int i = 0;

    printk("[%d: %s(", current->pid, s->s_name);
    goto pscl;

    while (info >>= 4) {
        printk(", ");

pscl:
        if (fmtspec[info & 0xf] != NULL)
            printk(fmtspec[info & 0xf], p->args[i]);
        else switch (info & 0xf) {
        case P_PULONG:
            printk("%lu", get_user_long((void *)p->args[i]));
            break;

        case P_PSLONG:
            printk("%ld", get_user_long((void *)p->args[i]));
            break;
#ifdef LATER
        case P_SLONG:       /* currently unused*/
            printk("%ld", *(long *)&p->args[i++]);
            break;

        case P_ULONG:       /* currently unused*/
            printk("%lu", *(unsigned long *)&p->args[i++]);
            break;
#endif
        }
        i++;

    }
    printk(")]");
}

void strace_retval(unsigned int retval)
{
    __ptask currentp = current;
    struct sc_info *s = syscall_info(currentp->t_regs.orig_ax);
    int n;
    static int max = 0;

    for (n=0; n<KSTACK_BYTES; n++)
        if (currentp->t_kstack[n] != 0x55)
            break;
    n = KSTACK_BYTES - n;
    if (n > max) max = n;
    printk("[%d:%s/ret=%d,ks=%d/%d]\n", currentp->pid, s->s_name, retval, n, max);
}
#endif

#ifdef CHECK_KSTACK
void check_kstack(void)
{
    int n;
    __ptask currentp = current;
    static int max = 0;

    /* Check for kernel stack overflow */
    if (currentp->kstack_magic != KSTACK_MAGIC) {
        printk("KSTACK(%d) KERNEL STACK OVERFLOW\n", current->pid);
        do_exit(SIGSEGV);
    }

    for (n=0; n<KSTACK_BYTES; n++)
        if (currentp->t_kstack[n] != 0x55)
            break;
    n = KSTACK_BYTES - n;
    if (n > currentp->kstack_max) {
        currentp->kstack_prevmax = currentp->kstack_max;
        currentp->kstack_max = n;
        if (n > max)
            max = n;
        printk("KSTACK(%d) sys_%7s max %3d prevmax %3d sysmax %3d",
            currentp->pid, syscall_info(currentp->t_regs.orig_ax)->s_name,
            currentp->kstack_max, currentp->kstack_prevmax, max);
        if (n == max) printk("*");
        printk("\n");
    }
}
#endif
