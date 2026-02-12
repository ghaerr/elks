#include <linuxmt/config.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/trace.h>
#include <linuxmt/mm.h>
#include <linuxmt/string.h>
#include <arch/segment.h>

/*
 * Kernel tracing functions for consistency checking and debugging support
 */

/* stringize the result of expansion of a macro argument */
#define str(bytes)  str2(bytes)
#define str2(bytes) #bytes

/*
 * Check that user SP is within proper range, called before every syscall.
 */
void check_ustack(void)
{
    segoff_t sp = current->t_regs.sp;
    segoff_t brk = current->t_endbrk;
    segoff_t stacklow = current->t_begstack - current->t_minstack;

    if (sp < brk) {
        printk("(%P)STACK OVERFLOW by %u\n", brk - sp);
        printk("CURBREAK %x, SP %x\n", brk, sp);
        do_exit(SIGSEGV);
    }
    if (sp < stacklow) {
        /* notification only, allow process to continue */
        printk("(%P)STACK USING %u UNUSED HEAP\n", stacklow - sp);
    }
    if (sp > current->t_begstack) {
        printk("(%P)STACK UNDERFLOW: SP %x BEGSTACK %x\n", sp, current->t_begstack);
        do_exit(SIGSEGV);
    }
}

#ifdef CONFIG_TRACE

/* The table describing the system calls has been moved to a separate
 * header file, and is included by the following include line.
 */
#include "strace.h"

static struct sc_info notimp = { "NOTIMP", 0};

static struct sc_info *syscall_info(unsigned int callno)
{
    struct sc_info *s;

    if (callno < sizeof(elks_table)/sizeof(struct sc_info)) {
        s = &elks_table[callno];
        if (s) return s;
    }
    return &notimp;
}

#ifdef CHECK_STRACE
static const char *fmtspec[] = {
    NULL,   "&0x%X", "0x%X",  "0x%X",
    "'%c'", "'%c'",  "\"%t\"","\"%t\"",
    "%u",   "%d",    "&%u",   "&%d",
    NULL,    NULL,    NULL,    NULL
};

struct sc_args {
    unsigned int args[3];    /* BX, CX, DX */
};

static void strace(void)
{
    struct sc_info *s = syscall_info(current->t_regs.orig_ax);
    unsigned int info = (s->s_info >> 4);
    struct sc_args *p = (struct sc_args *)&current->t_regs.bx;
    int i = 0;

    printk("[%P: %s(", s->s_name);
    goto pscl;

    while (info >>= 4) {
        printk(", ");

pscl:
        if (fmtspec[info & 0xf] != NULL)
            printk(fmtspec[info & 0xf], p->args[i]);
        else switch (info & 0xf) {
        case P_PSLONG:
            printk("%ld", get_user_long((void *)p->args[i]));
            break;
#if UNUSED
        case P_PULONG:      /* currently unused */
            printk("%lu", get_user_long((void *)p->args[i]));
            break;

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
#endif

#ifdef CHECK_ISTACK
/* calc interrupt stack usage */
void check_istack(void)
{
    int i;
    static int maxistack;

    for (i=0; i<INTRSTACK_BYTES/2; i++) {
        if (endistack[i] != 0)
            break;
    }
    i = (INTRSTACK_BYTES/2 - i) << 1;
    if (i > maxistack) {
        maxistack = i;
        printk("ISTACK MAX %d\n", i);
    }
}
#endif

#ifdef CHECK_KSTACK
/* calculate kernel stack usage per system call */
static void check_kstack(int n)
{
    struct sc_info *s;
    const char *warning = "";
    static int max;

#ifdef CHECK_ISTACK
    if (tracing & TRACE_ISTACK)
        check_istack();
#endif
    s = syscall_info(current->t_regs.orig_ax);
    if (s == &notimp)
        printk("KSTACK(%P) syscall %d NOTIMP\n", current->t_regs.orig_ax);
    if (n >= KSTACK_BYTES - KSTACK_GUARD)
        warning = " (OVERFLOW AT " str(KSTACK_BYTES) ")";
    if (n > current->kstack_max) {
        current->kstack_prevmax = current->kstack_max;
        current->kstack_max = n;
        if (n > max)
            max = n;
        if (current->kstack_prevmax != 0) {
            printk("KSTACK(%P) sys_%7s max %3d prevmax %3d sysmax %3d%s", s->s_name,
                current->kstack_max, current->kstack_prevmax, max, warning);
            if (n == max) printk("*");
            printk("\n");
        }
    }
}
#endif

/*
 * Called before syscall entry
 */
void trace_begin(void)
{
#if defined(CHECK_STRACE) || defined(CHECK_KSTACK)
    if (tracing & (TRACE_STRACE|TRACE_KSTACK))
        memset(current->t_kstack, 0x55, KSTACK_BYTES-32);
#endif
#ifdef CHECK_STRACE
    if (tracing & TRACE_STRACE)
        strace();
#endif
}

/*
 * Called after syscall return
 * Must be compiled with -fno-optimize-siblings-calls (no tail call optimization)
 * in order to protect top of stack return value since called from _irqit.
 */
void trace_end(unsigned int retval)
{
    int n;
    static int max;

    /* Check for kernel stack overflow */
    if (current->kstack_magic != KSTACK_MAGIC) {
        printk("KSTACK(%P) KERNEL STACK OVERFLOW\n");
        do_exit(SIGSEGV);
    }

    n = 0;
#if defined(CHECK_STRACE) || defined(CHECK_KSTACK)
    if (tracing & (TRACE_STRACE|TRACE_KSTACK)) {
        for (; n<KSTACK_BYTES/2; n++) {
        if (current->t_kstack[n] != 0x5555)
                break;
        }
        n = (KSTACK_BYTES/2 - n) << 1;
        if (n > max) max = n;
    }
#endif
#ifdef CHECK_STRACE
    if (tracing & TRACE_STRACE) {
        struct sc_info *s = syscall_info(current->t_regs.orig_ax);
        printk("[%P:%s/ret=%d,ks=%d/%d]\n", s->s_name, retval, n, max);
    }
#endif
#ifdef CHECK_KSTACK
    if (tracing & TRACE_KSTACK)
        check_kstack(n);
#endif
}

#endif /* CONFIG_TRACE */
