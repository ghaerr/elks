#ifdef CONFIG_STRACE

/* The table describing the system calls has been moved to a separate
 * header file, and is included by the following include line.
 */

#include "strace.h"

void print_syscall(register struct syscall_params *p, int retval)
{
    int tmpa, tent = 0;
    unsigned char i, tmpb;

    /* Scan elks_syscalls for the system call info */

    while ((elks_table[tent].s_num != 0) &&
	   (elks_table[tent].s_num != p->s_num))
	tent++;

    if (elks_table[tent].s_num)
	printk("Syscall not recognised: %u\n", p->s_num);
    else {

#ifdef STRACE_PRINTSTACK

	printk("[%d/%p: %d %s(", current->pid, current->t_regs.sp, p->s_num,
	       elks_table[tent].s_name);

#else

	printk("[%d: %s(", current->pid, elks_table[tent].s_name);

#endif

	for (i = 0; i < elks_table[tent].s_params; i++) {

	    if (i)
		printk(", ");

	    switch (elks_table[tent].t_param[i]) {

	    case P_DATA:
		printk("&0x%X", p->s_param[i]);

	    case P_NONE:
		break;

	    case P_POINTER:
	    case P_PDATA:
		printk("0x%X", p->s_param[i]);
		break;

	    case P_UCHAR:
	    case P_SCHAR:
		printk("'%c'", p->s_param[i]);
		break;

	    case P_STR:
		con_charout('\"');
		tmpa = p->s_param[i];
		while ((tmpb = get_fs_byte(tmpa++)))
		    con_charout(tmpb);
		con_charout('\"');
		break;

	    case P_PSTR:
		con_charout('&');
		con_charout('\"');
		tmpa = p->s_param[i];
		while ((tmpb = get_fs_byte(tmpa++)))
		    con_charout(tmpb);
		con_charout('\"');
		break;

	    case P_USHORT:
		printk("%u", p->s_param[i]);
		break;

	    case P_SSHORT:
		printk("%d", p->s_param[i]);
		break;

	    case P_PUSHORT:
		printk("&%u", p->s_param[i]);
		break;

	    case P_PSSHORT:
		printk("&%d", p->s_param[i]);
		break;

	    case P_SLONG:
		printk("%ld", p->s_param[i]);
		break;

	    case P_ULONG:
		printk("%lu", p->s_param[i]);
		break;

	    case P_PSLONG:
		printk("%ld", get_fs_long(p->s_param[i]));
		break;

	    case P_PULONG:
		printk("%lu", get_fs_long(p->s_param[i]));
		break;

	    default:
		break;
	    }

	}

    }
#ifdef STRACE_RETWAIT
    printk(") = %d]\n", retval);
#else
    p->s_name = elks_table[tent].s_name;
    printk(")]");
#endif
}

/* Funny how syscall_params just happens to match the layout of the system
 * call paramters on the stack, isn't it? :)
 */

int strace(struct syscall_params p)
{
    /* set up cur_sys */
    current->sc_info = p;
#ifndef STRACE_RETWAIT
    print_syscall(&current->sc_info, 0);
#endif
    /* First we check the kernel stack magic */
    if (current->t_kstackm != KSTACK_MAGIC)
	panic("Process %d had kernel stack overflow before syscall\n",
	      current->pid);
    return p.s_num;
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
