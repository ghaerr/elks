#include <linuxmt/autoconf.h>

#ifdef CONFIG_STRACE

#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/debug.h>
#include <linuxmt/config.h>
#include <linuxmt/wait.h>
#include <linuxmt/sched.h>

/* The table describing the system calls has been moved to a separate
 * header file, and is included by the following include line.
 */

#include "strace.h"

static char *fmtspec[] = {
    NULL,   "&0x%X", "0x%X",  "0x%X",
    "'%c'", "'%c'",  "\"%t\"","&\"%t\"",
    "%u",   "%d",    "&%u",   "&%d",
    "%lu",  "%ld",   NULL,    NULL,
};

void print_syscall(register struct syscall_params *p, int retval)
{
    register struct syscall_info *s;
    unsigned int tmpa;
    int i;

    if (p->s_num >= sizeof(elks_table)/sizeof(struct syscall_info))
	printk("Syscall not recognised: %u\n", p->s_num);
    else if ((((s = &elks_table[p->s_num])->s_params) & 0xf) > 5) {
	printk("Syscall not supported: nosys_%s\n", p->s_name);
    }
    else {

#ifdef STRACE_PRINTSTACK

	printk("[%d/%p: %d %12s(", current->pid, current->t_regs.sp,
	       p->s_num, s->s_name);

#else

	printk("[%d: %12s(", current->pid, s->s_name);

#endif

	i = 0;
	tmpa = s->s_params;
	goto pscl;
	while (tmpa >>= 4) {
	    printk(", ");

	 pscl:
	    if (fmtspec[tmpa & 0xf] != NULL) {
		printk(fmtspec[tmpa & 0xf], p->s_param[i]);
	    }
	    else switch (tmpa & 0xf) {

	    case P_PULONG:
		printk("%lu", get_user_long(p->s_param[i]));
		break;

	    case P_PSLONG:
		printk("%ld", get_user_long(p->s_param[i]));

	    default:
		break;
	    }
	    i++;

	}

    }
#ifdef STRACE_RETWAIT
    printk(") = %d]\n", retval);
#else
    p->s_name = s->s_name;
    printk(")]");
#endif
}

/* Funny how syscall_params just happens to match the layout of the system
 * call paramters on the stack, isn't it? :)
 */

void strace(struct syscall_params p)
{
    /* First we check the kernel stack magic */
    if (current->t_kstackm != KSTACK_MAGIC)
	panic("Process %d had kernel stack overflow before syscall\n",
	      current->pid);
    /* set up cur_sys */
    current->sc_info = p;
#ifndef STRACE_RETWAIT
    print_syscall(&current->sc_info, 0);
#endif
}

void ret_strace(unsigned int retval)
{
#ifdef STRACE_RETWAIT
    print_syscall(&current->sc_info, retval);
#else
    printk("[%d:%s/ret=%d]\n", current->pid, current->sc_info.s_name, retval);
#endif
}

#endif
