/*
 *  linux/kernel/sys.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/utsname.h>
#ifdef NOT_YET
#include <linuxmt/times.h>
#include <linuxmt/param.h>
#endif
#include <linuxmt/signal.h>
#include <linuxmt/string.h>
#include <linuxmt/stat.h>
#include <linuxmt/mm.h>

#include <arch/segment.h>
#include <arch/io.h>
/*
 * this indicates whether you can reboot with ctrl-alt-del: the default is yes
 */

static int C_A_D = 1;

extern void hard_reset_now();
extern int sys_kill();

/*
 * Reboot system call: for obvious reasons only root may call it,
 * and even root needs to set up some magic numbers in the registers
 * so that some mistake won't make this reboot the whole machine.
 * You can also set the meaning of the ctrl-alt-del-key here.
 *
 * reboot doesn't sync: do that yourself before calling this.
 */
 
int sys_reboot(magic,magic_too,flag)
unsigned int magic;
unsigned int magic_too;
int flag;
{
 	if (!suser())
		return -EPERM;
	if (magic != 0x1D1E || magic_too != 0xC0DE )
		return -EINVAL;
	if (flag == 0x0123)
		hard_reset_now();
	else if (flag == 0x4567)
		C_A_D = 1;
	else if (!flag)
		C_A_D = 0;
	else if (flag == 0x6789) {
		printk("System halted\n");
		sys_kill(-1, SIGKILL);
		do_exit(0);
	} else
		return -EINVAL;
	return (0);
}

/*
 * This function gets called by ctrl-alt-del - ie the keyboard interrupt.
 * As it's called within an interrupt, it may NOT sync: the only choice
 * is whether to reboot at once, or just ignore the ctrl-alt-del.
 */
 
void ctrl_alt_del()
{
	if (C_A_D)
		hard_reset_now();
	else
		kill_proc(1, SIGINT, 1);
}
	

/*
 * setgid() is implemented like SysV w/ SAVED_IDS 
 */

int sys_setgid(gid)
gid_t gid;
{
	register __ptask currentp = current;
	int old_egid = currentp->egid;

	if (suser())
		currentp->gid = currentp->egid = currentp->sgid = gid;
	else if ((gid == currentp->gid) || (gid == currentp->sgid))
		currentp->egid = gid;
	else
		return -EPERM;
	if (currentp->egid != old_egid)
		currentp->dumpable = 0;
	return 0;
}

int sys_getuid()
{
	return current->uid;
}

int sys_getpid()
{
	return current->pid;
}

int sys_umask(mask)
int mask;
{
	int old = current->fs.umask;

	current->fs.umask = mask & S_IRWXUGO;
	return (old);
}

/*
 * setuid() is implemented like SysV w/ SAVED_IDS 
 * 
 * Note that SAVED_ID's is deficient in that a setuid root program
 * like sendmail, for example, cannot set its uid to be a normal 
 * user and then switch back, because if you're root, setuid() sets
 * the saved uid too.  If you don't like this, blame the bright people
 * in the POSIX committee and/or USG.  Note that the BSD-style setreuid()
 * will allow a root program to temporarily drop privileges and be able to
 * regain them by swapping the real and effective uid.  
 */

int sys_setuid(uid)
uid_t uid;
{
	register __ptask currentp = current;
	int old_euid = currentp->euid;

	if (suser())
		currentp->uid = currentp->euid = currentp->suid = uid;
	else if ((uid == currentp->uid) || (uid == currentp->suid))
		currentp->euid = uid;
	else
		return -EPERM;
	if (currentp->euid != old_euid)
		currentp->dumpable = 0;
	return(0);
}

#ifdef NOT_YET
int sys_times(struct tms * tbuf)
{
	if (tbuf) {
		int error = verify_area(VERIFY_WRITE,tbuf,sizeof *tbuf);
		if (error)
			return error;
		put_user_long(current->utime,&tbuf->tms_utime);
		put_user_long(current->stime,&tbuf->tms_stime);
		put_user_long(current->cutime,&tbuf->tms_cutime);
		put_user_long(current->cstime,&tbuf->tms_cstime);
	}
	return jiffies;
}
#endif

/*
 * This needs some heavy checking ...
 * I just haven't the stomach for it. I also don't fully
 * understand sessions/pgrp etc. Let somebody who does explain it.
 *
 * OK, I think I have the protection semantics right.... this is really
 * only important on a multi-user system anyway, to make sure one user
 * can't send a signal to a process owned by another.  -TYT, 12/12/91
 *
 * Auch. Had to add the 'did_exec' flag to conform completely to POSIX.
 * LBT 04.03.94
 */

#ifdef NOT_YET

int sys_setpgid(pid,pgid)
pid_t pid;
pid_t pgid;
{
	register struct task_struct * p;

	if (!pid)
		pid = current->pid;
	if (!pgid)
		pgid = pid;
	if (pgid < 0)
		return -EINVAL;
	for_each_task(p) {
		if (p->pid == pid)
			goto found_task;
	}
	return -ESRCH;

found_task:
	if (p->p_pptr == current || p->p_opptr == current) {
		if (p->session != current->session)
			return -EPERM;
		if (p->did_exec)
			return -EACCES;
	} else if (p != current)
		return -ESRCH;
	if (p->leader)
		return -EPERM;
	if (pgid != pid) {
		struct task_struct * tmp;
		for_each_task (tmp) {
			if (tmp->pgrp == pgid &&
			 tmp->session == current->session)
				goto ok_pgid;
		}
		return -EPERM;
	}

ok_pgid:
	p->pgrp = pgid;
	return 0;
}
#endif
#if 0
int sys_getpgid(pid)
pid_t pid;
{
	register struct task_struct * p;

	if (!pid)
		return current->pgrp;
	for_each_task(p) {
		if (p->pid == pid)
			return p->pgrp;
	}
	return -ESRCH;
}

int sys_getpgrp()
{
	return current->pgrp;
}

int sys_getsid(pid)
pid_t pid;
{
	register struct task_struct * p;

	if (!pid)
		return current->session;
	for_each_task(p) {
		if (p->pid == pid)
			return p->session;
	}
	return -ESRCH;

}
#endif

int sys_setsid()
{
	register __ptask currentp = current;

	if (currentp->session == currentp->pid) {
		return -EPERM;
	}
	currentp->session = currentp->pgrp = currentp->pid;
	currentp->tty = NULL;
/*	currentp->tty_old_pgrp = 0; */
	return currentp->pgrp;
}

/*
 * Supplementary group ID's
 */
#ifdef CONFIG_SUPPLEMENTARY_GROUPS
int sys_getgroups(gidsetsize,grouplist)
int gidsetsize;
gid_t *grouplist;
{
	int i, retval;
	register gid_t *groups;

	groups=current->groups;
	for (i = 0 ; (i < NGROUPS) && (*groups != NOGROUP) ; i++, groups++) {
		if (!gidsetsize)
			continue;
		if (i >= gidsetsize)
			break;
		if ((retval = verified_memcpy_tofs(grouplist, groups, sizeof(gid_t))) != 0)
			return retval;
		grouplist++;
	}
	return(i);
}

int sys_setgroups(gidsetsize, grouplist)
int gidsetsize;
gid_t *grouplist;
{
	int	i, retval;

	if (!suser())
		return -EPERM;
	if (gidsetsize > NGROUPS)
		return -EINVAL;
	for (i = 0; i < gidsetsize; i++, grouplist++) {
		gid_t g;
		if (retval = verified_memcpy_fromfs(&g, grouplist,sizeof(gid_t)))
			return retval;
		current->groups[i] = g;
	}
	if (i < NGROUPS)
		current->groups[i] = NOGROUP;
	return 0;
}

int in_group_p(grp)
gid_t grp;
{
	int	i;

	if (grp == current->egid)
		return 1;

	for (i = 0; i < NGROUPS; i++) {
		if (current->groups[i] == NOGROUP)
			break;
		if (current->groups[i] == grp)
			return 1;
	}
	return 0;
}
#else  /* CONFIG_SUPPLEMENTARY_GROUPS */
int in_group_p(grp)
gid_t grp;
{
	if (grp == current->egid) {
		return 1;
	}
}
#endif /* CONFIG_SUPPLEMENTARY_GROUPS */

