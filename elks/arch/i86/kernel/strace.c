#include <linuxmt/autoconf.h>

#ifdef CONFIG_STRACE

#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/debug.h>
#include <linuxmt/config.h>
#include <linuxmt/strace.h>
#include <linuxmt/wait.h>
#include <linuxmt/sched.h>

/* ELKS (v. >= 0.0.49) system call layout */

/* 
 * syscall_info table format : 
 * {system call #, "sys_call_name", # of parameters, {parameter types,
 * explained in, strace.h}}, 
 * 
 * Not finished yet (need to fill in parameter types) but enough to prove
 * that the concept works.
 */

struct syscall_info elks_table[] = {
	{1, "sys_exit", 0, {P_NONE, P_NONE, P_NONE}},
	{2, "sys_fork", 0, {P_NONE, P_NONE, P_NONE}},
	{3, "sys_read", 3, {P_UINT, P_PDATA, P_UINT}},
	{4, "sys_write", 3, {P_UINT, P_PDATA, P_UINT}},
	{5, "sys_open", 3, {P_PSTR, P_SINT, P_SINT}},
	{6, "sys_close", 1, {P_UINT, P_NONE, P_NONE}},
	{7, "sys_wait", 3, {P_UINT, P_UINT, P_UINT}},
	{8, "nsys_creat", 0, {P_NONE, P_NONE, P_NONE}},
	{9, "sys_link", 2, {P_PSTR, P_PSTR, P_NONE}},
	{10, "sys_unlink", 1, {P_PSTR, P_NONE, P_NONE}},
	{11, "sys_exec", 3, {P_PSTR, P_PDATA, P_UINT}},
	{12, "sys_chdir", 1, {P_PSTR, P_NONE, P_NONE}},
	{13, "sys_time", 0, {P_NONE, P_NONE, P_NONE}},
	{14, "sys_mknod", 3, {P_PSTR, P_UINT, P_UINT}},
	{15, "sys_chmod", 2, {P_PSTR, P_UINT, P_NONE}},
	{16, "sys_chown", 3, {P_PSTR, P_UINT, P_UINT}},
	{17, "sys_brk", 1, {P_UINT, P_NONE, P_NONE}},
	{18, "sys_stat", 2, {P_PSTR, P_PDATA, P_NONE}},
	{19, "sys_lseek", 3, {P_UINT, P_PDATA, P_UINT}},
	{20, "sys_getpid", 0, {P_NONE, P_NONE, P_NONE}},
	{21, "sys_mount", 3, {P_PSTR, P_PSTR, P_PSTR}},
	{22, "sys_umount", 0, {P_NONE, P_NONE, P_NONE}},
	{23, "sys_setuid", 0, {P_NONE, P_NONE, P_NONE}},
	{24, "sys_getuid", 0, {P_NONE, P_NONE, P_NONE}},
	{25, "sys_stime", 0, {P_NONE, P_NONE, P_NONE}},
	{26, "sys_ptrace", 0, {P_NONE, P_NONE, P_NONE}},
	{27, "sys_alarm", 0, {P_NONE, P_NONE, P_NONE}},
	{28, "sys_fstat", 0, {P_NONE, P_NONE, P_NONE}},
	{29, "sys_pause", 0, {P_NONE, P_NONE, P_NONE}},
	{30, "sys_utime", 0, {P_NONE, P_NONE, P_NONE}},
	{31, "sys_chroot", 0, {P_NONE, P_NONE, P_NONE}},
	{32, "sys_vfork", 0, {P_NONE, P_NONE, P_NONE}},
	{33, "sys_access", 0, {P_NONE, P_NONE, P_NONE}},
	{34, "sys_nice", 0, {P_NONE, P_NONE, P_NONE}},
	{35, "sys_sleep", 0, {P_NONE, P_NONE, P_NONE}},
	{36, "sys_sync", 0, {P_NONE, P_NONE, P_NONE}},
	{37, "sys_kill", 0, {P_NONE, P_NONE, P_NONE}},
	{38, "sys_rename", 2, {P_PSTR, P_PSTR, P_NONE}},
	{39, "sys_mkdir", 2, {P_PSTR, P_UINT, P_NONE}},
	{40, "sys_rmdir", 1, {P_PSTR, P_NONE, P_NONE}},
	{41, "sys_dup", 0, {P_NONE, P_NONE, P_NONE}},
	{42, "sys_pipe", 0, {P_NONE, P_NONE, P_NONE}},
	{43, "sys_times", 0, {P_NONE, P_NONE, P_NONE}},
	{44, "sys_profil", 0, {P_NONE, P_NONE, P_NONE}},
	{45, "sys_dup2", 0, {P_NONE, P_NONE, P_NONE}},
	{46, "sys_setgid", 0, {P_NONE, P_NONE, P_NONE}},
	{47, "sys_getgid", 0, {P_NONE, P_NONE, P_NONE}},
	{48, "sys_signal", 0, {P_NONE, P_NONE, P_NONE}},
	{49, "sys_getinfo", 0, {P_NONE, P_NONE, P_NONE}},
	{50, "sys_fcntl", 0, {P_NONE, P_NONE, P_NONE}},
	{51, "sys_acct", 0, {P_NONE, P_NONE, P_NONE}},
	{52, "sys_phys", 0, {P_NONE, P_NONE, P_NONE}},
	{53, "sys_lock", 0, {P_NONE, P_NONE, P_NONE}},
	{54, "sys_ioctl", 3, {P_UINT, P_UINT, P_PULONG}},
	{55, "sys_reboot", 0, {P_NONE, P_NONE, P_NONE}},
	{56, "sys_mpx", 0, {P_NONE, P_NONE, P_NONE}},
	{57, "sys_lstat", 2, {P_PSTR, P_PDATA, P_NONE}},
	{58, "sys_symlink", 0, {P_NONE, P_NONE, P_NONE}},
	{59, "sys_readlink", 0, {P_NONE, P_NONE, P_NONE}},
	{60, "sys_umask", 0, {P_NONE, P_NONE, P_NONE}},
	{61, "sys_settimeofday", 0, {P_NONE, P_NONE, P_NONE}},
	{62, "sys_gettimeofday", 0, {P_NONE, P_NONE, P_NONE}},
	{63, "sys_wait4", 0, {P_NONE, P_NONE, P_NONE}},
	{64, "sys_readdir", 3, {P_UINT, P_PDATA, P_UINT}},
	{68, "setsid", 0, {P_NONE, P_NONE, P_NONE}},
	{69, "sys_socket", 3, {P_UINT, P_UINT, P_UINT}},
	{0, "no_sys", 0, {P_NONE, P_NONE, P_NONE}},
};

void print_syscall(p, retval)
register struct syscall_params *p;
int retval;
{
	int tent = 0;
	unsigned char i, tmpb;
	int tmpa;

	/* Scan elks_syscalls for the system call info */

	while ( (elks_table[tent].s_num != 0) && 
		(elks_table[tent].s_num != p->s_num)) 
	{
		tent++;
	}

#ifdef STRACE_PRINTSTACK
	printk("[%d/%p: %d %s(", current->pid, current->t_regs.sp, p->s_num,elks_table[tent].s_name); 
#else
	printk("[%d: %s(", current->pid, elks_table[tent].s_name);	
#endif

	for (i = 0; i < elks_table[tent].s_params; i++) {
		if (i) printk(", ");
		switch (elks_table[tent].t_param[i]) {
			case P_SINT:
			case P_UINT:
				printk("%d", p->s_param[i]);
				break;
			case P_LONG:
			case P_ULONG:
				printk("%ld", p->s_param[i]);
			case P_PDATA:
			case P_POINTER:
				printk("0x%x", p->s_param[i]);
				break;
			case P_PLONG:
			case P_PULONG:
				printk("%ld", get_fs_long(p->s_param[i]));
				break;
			case P_PSTR:
				con_charout('\"');
				tmpa = p->s_param[i];
				while ((tmpb = get_fs_byte(tmpa++)) != 0)
					con_charout(tmpb);
				con_charout('\"');
			case P_NONE:
			default:
				break;
		};
	}
	#ifdef STRACE_RETWAIT
		printk(") = %d]\n", retval);
	#else
		p->s_name = elks_table[tent].s_name; 
		printk(")]");
	#endif
}

/* Funny how syscall_params just happens to match the layout of the system
 * call paramters on the stack, ain't it? :) */

int strace(p)
struct syscall_params p;
{
	/* set up cur_sys */
	current->sc_info = p;
#ifndef STRACE_RETWAIT
	print_syscall(&current->sc_info, 0);
#endif
	/* First we check the kernel stack magic */
	if (current->t_kstackm != KSTACK_MAGIC)
	   panic("Process %d had kernel stack overflow before syscall\n", current->pid); 
	return p.s_num;
}

void ret_strace(retval)
unsigned int retval;
{
#ifdef STRACE_RETWAIT
	print_syscall(&current->sc_info, retval);
#else
	printk("[%d:%s/ret=%d]\n", current->pid, current->sc_info.s_name, retval);
#endif
}

#endif
