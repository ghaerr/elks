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
 * Indicates whether you can reboot with ctrl-alt-del: the default is yes
 */

static int C_A_D = 1;

extern void hard_reset_now(void);
extern int sys_kill(sig_t,pid_t);

/*
 * Reboot system call: for obvious reasons only root may call it, and even
 * root needs to set up some magic numbers in the registers so that some
 * mistake won't make this reboot the whole machine.
 *
 * You can also set the meaning of the ctrl-alt-del-key here.
 *
 * reboot doesn't sync: do that yourself before calling this.
 */

int sys_reboot(unsigned int magic, unsigned int magic_too, int flag)
{
    if (!suser()) {
	return -EPERM;
    }

    if ((magic == 0x1D1E) && (magic_too == 0xC0DE)) {
	register int *pflag = (int *)flag;

	switch((int)pflag) {
	    case 0x4567:
		pflag = (int *)1;
	    case 0:
		C_A_D = (int)pflag;
		return 0;
	    case 0x0123:
		hard_reset_now();
	    case 0x6789:
		printk("System halted\n");
		sys_kill(-1, SIGKILL);
		do_exit(0);
#ifdef CONFIG_APM
	    case 0xDEAD:
		apm_shutdown_now();
		printk("APM shutdown failed\n");
#endif
	}
    }

    return -EINVAL;
}

/*
 * This function gets called by ctrl-alt-del - ie the keyboard interrupt.
 * As it's called within an interrupt, it may NOT sync: the only choice
 * is whether to reboot at once, or just ignore the ctrl-alt-del.
 */

void ctrl_alt_del(void)
{
    if (C_A_D)
	hard_reset_now();

    kill_process(1, (sig_t) SIGINT, 1);
}

#ifdef CONFIG_SYS_VERSION

/*
 * This function returns the version number associated with this kernel.
 */

int sys_knlvsn(char *vsn)
{
    register char *p = system_utsname.release;

    return verified_memcpy_tofs(vsn, p, strlen(p) + 1);
}

#endif

/*
 * setgid() is implemented like SysV w/ SAVED_IDS
 */

int sys_setgid(gid_t gid)
{
    register __ptask currentp = current;
    gid_t old_egid = currentp->egid;

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

uid_t sys_getuid(void)
{
    return current->uid;
}

pid_t sys_getpid(void)
{
    return current->pid;
}

unsigned short int sys_umask(unsigned short int mask)
{
    register __ptask currentp = current;
    unsigned short int old;

    old = currentp->fs.umask;
    currentp->fs.umask = mask & ((unsigned short int) S_IRWXUGO);
    return old;
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

int sys_setuid(uid_t uid)
{
    register __ptask currentp = current;
    uid_t old_euid = currentp->euid;

    if (suser())
	currentp->uid = currentp->euid = currentp->suid = uid;
    else if ((uid == currentp->uid) || (uid == currentp->suid))
	currentp->euid = uid;
    else
	return -EPERM;
    if (currentp->euid != old_euid)
	currentp->dumpable = 0;
    return 0;
}

#ifdef NOT_YET

int sys_times(struct tms *tbuf)
{
    if (tbuf) {
	int error = verify_area(VERIFY_WRITE, tbuf, sizeof *tbuf);

	if (error)
	    return error;
	put_user_long(current->utime, &tbuf->tms_utime);
	put_user_long(current->stime, &tbuf->tms_stime);
	put_user_long(current->cutime, &tbuf->tms_cutime);
	put_user_long(current->cstime, &tbuf->tms_cstime);
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

int sys_setpgid(pid_t pid, pid_t pgid)
{
    register struct task_struct *p;

    if (!pid)
	pid = current->pid;

    if (!pgid)
	pgid = pid;

    if (pgid < 0)
	return -EINVAL;

    for_each_task(p) {
	if (p->pid == pid) {

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
		struct task_struct *tmp;

		for_each_task(tmp) {
		    if ((tmp->pgrp == pgid)
			&& (tmp->session == current->session)) {
			goto ok_pgid:
		    }
		}
		return -EPERM;
	    }

	ok_pgid:
	    p->pgrp = pgid;

	    return 0;
	}
    }
    return -ESRCH;
}

#endif

#if 0

int sys_getpgid(pid_t pid)
{
    register struct task_struct *p;

    if (!pid)
	return current->pgrp;
    for_each_task(p)
	if (p->pid == pid)
	    return p->pgrp;
    return -ESRCH;
}

int sys_getpgrp(void)
{
    return current->pgrp;
}

int sys_getsid(pid_t pid)
{
    register struct task_struct *p;

    if (!pid)
	return current->session;
    for_each_task(p)
	if (p->pid == pid)
	    return p->session;
    return -ESRCH;

}

#endif

int sys_setsid(void)
{
    register __ptask currentp = current;

    if (currentp->session == currentp->pid) {
	return -EPERM;
    }
    currentp->session = currentp->pgrp = currentp->pid;
    currentp->tty = NULL;

#if 0
    currentp->tty_old_pgrp = 0;
#endif

    return currentp->pgrp;
}

/*
 * Supplementary group ID's
 */

#ifdef CONFIG_SUPPLEMENTARY_GROUPS

int sys_getgroups(int gidsetsize, gid_t * grouplist)
{
    register gid_t *pg = current->groups;
    register char *pi = 0;

    while ((pg[(int)pi] != NOGROUP) && (((int)(++pi)) < NGROUPS));

    if (pi && gidsetsize) {
	if (((int)pi) > gidsetsize) {
	    return -EINVAL;
	}
	if (verified_memcpy_tofs(grouplist, pg, ((int)pi) * sizeof(gid_t)) != 0)
	    return -EFAULT;

    }

    return (int)pi;
}

int sys_setgroups(int gidsetsize, gid_t * grouplist)
{
    register gid_t *pg;
    register gid_t *lg;

    if (!suser())
        return -EPERM;

#if 1
    /* semantics change.. return EINVAL for gidsetsize < 0. */
    if (((unsigned int)gidsetsize) > NGROUPS)
        return -EINVAL;

    pg = current->groups;
    lg = pg + gidsetsize;
#else
    if (gidsetsize > NGROUPS)
        return -EINVAL;

    pg = current->groups;
    lg = pg + ((gidsetsize >= 0) ? gidsetsize : 0);
#endif

    if (lg > pg) {
	if (verified_memcpy_fromfs(pg, grouplist, ((char *)lg) - ((char *)pg)))
	    return -EFAULT;
    }

    if (gidsetsize < NGROUPS) {
	*lg = NOGROUP;
    }

    return 0;
}

int in_group_p(gid_t grp)
{
    register char *p = (char *) current;

    if (grp != ((__ptask) p)->egid) {
	register gid_t *pg = ((__ptask) p)->groups - 1;

	p = (char *)(pg + NGROUPS);

	do {
	    if ((++pg > ((gid_t *) p)) || (*pg == NOGROUP)) {
		return 0;
	    }
	} while (*pg != grp);
    }

    return 1;
}

#else

int in_group_p(gid_t grp)
{
    return (grp == current->egid);
}

#endif
