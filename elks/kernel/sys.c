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

extern void adjust_clock();
#if 0
sys_ni_syscall()
{
	return -ENOSYS;
}
#endif

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
 * Unprivileged users may change the real gid to the effective gid
 * or vice versa.  (BSD-style)
 *
 * If you set the real gid at all, or set the effective gid to a value not
 * equal to the real gid, then the saved gid is set to the new effective gid.
 *
 * This makes it possible for a setgid program to completely drop its
 * privileges, which is often a useful assertion to make when you are doing
 * a security audit over a program.
 *
 * The general idea is that a program which uses just setregid() will be
 * 100% compatible with BSD.  A program which uses just setgid() will be
 * 100% compatible with POSIX w/ Saved ID's. 
 */
#if 0
int sys_setregid(rgid,egid)
gid_t rgid;
gid_t egid;
{
	int old_rgid = current->gid;
	int old_egid = current->egid;

	if (rgid != (gid_t) -1) {
		if ((old_rgid == rgid) ||
		    (current->egid==rgid) ||
		    suser())
			current->gid = rgid;
		else
			return(-EPERM);
	}
	if (egid != (gid_t) -1) {
		if ((old_rgid == egid) ||
		    (current->egid == egid) ||
		    (current->sgid == egid) ||
		    suser())
			current->egid = egid;
		else {
			current->gid = old_rgid;
			return(-EPERM);
		}
	}
	if (rgid != (gid_t) -1 ||
	    (egid != (gid_t) -1 && egid != old_rgid))
		current->sgid = current->egid;
#ifdef NEED_CORE
	if (current->egid != old_egid)
		current->dumpable = 0;
#endif
	return 0;
}
#endif
/*
 * setgid() is implemented like SysV w/ SAVED_IDS 
 */

int sys_setgid(gid)
gid_t gid;
{
	int old_egid = current->egid;

	if (suser())
		current->gid = current->egid = current->sgid = gid;
	else if ((gid == current->gid) || (gid == current->sgid))
		current->egid = gid;
	else
		return -EPERM;
#ifdef NEED_CORE
	if (current->egid != old_egid)
		current->dumpable = 0;
#endif
	return 0;
}
#if 0
int sys_acct()
{
	return -ENOSYS;
}
#endif
int sys_getuid()
{
	return current->uid;
}

int sys_getpid()
{
	return current->pid;
}

/*
 * Why do these exist?  Binary compatibility with some other standard?
 * If so, maybe they should be moved into the appropriate arch
 * directory.
 */
#if 0
int sys_phys()
{
	return -ENOSYS;
}

int sys_lock()
{
	return -ENOSYS;
}

int sys_mpx()
{
	return -ENOSYS;
}

int sys_ulimit()
{
	return -ENOSYS;
}
#endif
/*
 * Unprivileged users may change the real uid to the effective uid
 * or vice versa.  (BSD-style)
 *
 * If you set the real uid at all, or set the effective uid to a value not
 * equal to the real uid, then the saved uid is set to the new effective uid.
 *
 * This makes it possible for a setuid program to completely drop its
 * privileges, which is often a useful assertion to make when you are doing
 * a security audit over a program.
 *
 * The general idea is that a program which uses just setreuid() will be
 * 100% compatible with BSD.  A program which uses just setuid() will be
 * 100% compatible with POSIX w/ Saved ID's. 
 */
#if 0
int sys_setreuid(ruid,euid)
uid_t ruid;
uid_t euid;
{
	int old_ruid = current->uid;
	int old_euid = current->euid;

	if (ruid != (uid_t) -1) {
		if ((old_ruid == ruid) || 
		    (current->euid==ruid) ||
		    suser())
			current->uid = ruid;
		else
			return(-EPERM);
	}
	if (euid != (uid_t) -1) {
		if ((old_ruid == euid) ||
		    (current->euid == euid) ||
		    (current->suid == euid) ||
		    suser())
			current->euid = euid;
		else {
			current->uid = old_ruid;
			return(-EPERM);
		}
	}
	if (ruid != (uid_t) -1 ||
	    (euid != (uid_t) -1 && euid != old_ruid))
		current->suid = current->euid;
#ifdef NEED_CORE
	if (current->euid != old_euid)
		current->dumpable = 0;
#endif
	return 0;
}
#endif
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
	int old_euid = current->euid;

	if (suser())
		current->uid = current->euid = current->suid = uid;
	else if ((uid == current->uid) || (uid == current->suid))
		current->euid = uid;
	else
		return -EPERM;
#ifdef NEED_CORE
	if (current->euid != old_euid)
		current->dumpable = 0;
#endif
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
#ifdef NOT_YET	
int sys_setsid()
{
	if (current->leader)
		return -EPERM;
	current->leader = 1;
	current->session = current->pgrp = current->pid;
	current->tty = NULL;
	current->tty_old_pgrp = 0;
	return current->pgrp;
}

#endif

/*
 * Supplementary group ID's
 */
#if 0 
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
#endif
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
#if 0
int sys_uname(name)
struct utsname * name;
{
	int error;

	if (!name)
		return -EFAULT;
	error = verified_memcpy_tofs(name,&system_utsname,sizeof(*name))
	return error;
}


int sys_sethostname(name,len)
char *name;
int len;
{
	int error;

	if (!suser())
		return -EPERM;
	if (len < 0 || len >= sizeof(system_utsname.nodename))
		return -EINVAL;
	if (error = verified_memcpy_fromfs(system_utsname.nodename, name, len))
		return error;
	system_utsname.nodename[len] = 0;
	return 0;
}

int sys_gethostname(name,len)
char *name;
int len;
{
	int error;

	if (len < 0)
		return -EINVAL;
	i = 1+strlen(system_utsname.nodename);
	if (i > len)
		i = len;
	error = verified_memcpy_tofs(name, system_utsname.nodename, i))
	return error;
}
#endif

int sys_umask(mask)
int mask;
{
	int old = current->fs.umask;

	current->fs.umask = mask & S_IRWXUGO;
	return (old);
}
