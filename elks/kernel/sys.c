/*
 *  linux/kernel/sys.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/utsname.h>
#include <linuxmt/signal.h>
#include <linuxmt/string.h>
#include <linuxmt/stat.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>
#include <arch/io.h>

/*
 * Indicates whether you can reboot with ctrl-alt-del: the default is yes
 */

static int C_A_D = 1;

extern int sys_kill(pid_t, sig_t);

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
    if (!suser())
	return -EPERM;

    if (magic == 0x1D1E && magic_too == 0xC0DE) {
	switch(flag) {
	    case 0x4567:
		flag = 1;
		/* fall through*/
	    case 0:
		C_A_D = flag;
		return 0;
	    case 0x0123:		/* reboot*/
		hard_reset_now();
		printk("Reboot failed\n");
		/* fall through*/
	    case 0x6789:		/* shutdown*/
		sys_kill(1, SIGKILL);
		sys_kill(-1, SIGKILL);
		printk("System halted\n");
		do_exit(0);
		/* no return*/
	    case 0xDEAD:		/* poweroff*/
		apm_shutdown_now();
		printk("APM shutdown failed\n");
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

/*
 * This function returns the version number associated with this kernel.
 */

int sys_uname(struct utsname *utsname)
{
    return verified_memcpy_tofs(utsname, &system_utsname, sizeof(struct utsname));
}

/*
 * setgid() is implemented like SysV w/ SAVED_IDS
 */

int sys_setgid(gid_t gid)
{
    if (suser())
	current->gid = current->egid = current->sgid = gid;
    else if ((gid == current->gid) || (gid == current->sgid))
	current->egid = gid;
    else
	return -EPERM;
    return 0;
}

static int twovalues(int retval, int *copyval, int *copyaddr)
{
    if (verified_memcpy_tofs(copyaddr, copyval, sizeof(int)) != 0)
	return -EFAULT;
    return retval;
}

uid_t sys_getuid(int *euid)
{
    return twovalues(current->uid, (int *)&current->euid, euid);
}

uid_t sys_getgid(int *egid)
{
    return twovalues(current->gid, (int *)&current->egid, egid);
}

pid_t sys_getpid(int *ppid)
{
    return twovalues(current->pid, (int *)&current->ppid, ppid);
}

unsigned short int sys_umask(mode_t mask)
{
    mode_t old;

    old = current->fs.umask;
    current->fs.umask = mask & S_IRWXUGO;
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
    if (suser())
	current->uid = current->euid = current->suid = uid;
    else if ((uid == current->uid) || (uid == current->suid))
	current->euid = uid;
    else
	return -EPERM;
    return 0;
}

int sys_setsid(void)
{
    if (current->session == current->pid)
	return -EPERM;
    debug_tty("SETSID pgrp %P\n");
    current->session = current->pgrp = current->pid;
    current->tty = NULL;

    return current->pgrp;
}

#if UNUSED
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
	if (p->pid == pid && p->state != TASK_UNUSED) {

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
		    if ((tmp->pgrp == pgid && tmp->state != TASK_UNUSED)
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

int sys_getpgid(pid_t pid)
{
    register struct task_struct *p;

    if (!pid)
	return current->pgrp;
    for_each_task(p)
	if (p->pid == pid && p->state != TASK_UNUSED)
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
	if (p->pid == pid && p->state != TASK_UNUSED)
	    return p->session;
    return -ESRCH;

}
#endif

#ifdef CONFIG_SUPPLEMENTARY_GROUPS
/*
 * Supplementary group ID's
 */

#define NOGROUP     0xFFFF

int sys_getgroups(int gidsetsize, gid_t * grouplist)
{
    register gid_t *pg = current->groups;
    int i = 0;

    while ((pg[i] != NOGROUP) && (++i < NGROUPS))
	continue;

    if (i && gidsetsize) {
	if (i > gidsetsize) {
	    return -EINVAL;
	}
	if (verified_memcpy_tofs(grouplist, pg, i * sizeof(gid_t)) != 0)
	    return -EFAULT;

    }

    return i;
}

int sys_setgroups(int gidsetsize, gid_t * grouplist)
{
    register gid_t *pg;
    register gid_t *lg;

    if (!suser())
        return -EPERM;

    if (((unsigned int)gidsetsize) > NGROUPS)
        return -EINVAL;

    pg = current->groups;
    lg = pg + gidsetsize;

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
    gid_t *pg;
    char *p;

    if (grp != (current->egid) {
	pg = current->groups - 1;
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
