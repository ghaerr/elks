/*
 *  linux/kernel/signal.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/debug.h>

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/signal.h>
#include <linuxmt/errno.h>
#include <linuxmt/wait.h>
#include <linuxmt/mm.h>

#include <arch/segment.h>

static void generate(int sig, register struct task_struct *p)
{
    __sighandler_t sa = p->sig.action[sig - 1].sa_handler;

    if((sa == SIG_IGN) || (sa == SIG_DFL) && (sig == SIGCONT
	|| sig == SIGCHLD || sig == SIGWINCH
#ifndef SMALLSIG
	|| sig == SIGURG
#endif
	))
	return;
    debug1("SIGNAL: Generating sig %d.\n", sig);
    p->signal |= (((sigset_t) 1) << (sig - 1));
    if ((p->state == TASK_INTERRUPTIBLE) /* && (p->signal & ~p->blocked) */ ) {
	debug("SIGNAL: Waking up.\n");
	wake_up_process(p);
    }
}

int send_sig(sig_t sig, register struct task_struct *p, int priv)
{
    register __ptask currentp = current;
    debug1("SIGNAL: Killing with sig %d.\n", sig);
    if (!priv && ((sig != SIGCONT) || (currentp->session != p->session)) &&
	(currentp->euid ^ p->suid) && (currentp->euid ^ p->uid) &&
	(currentp->uid ^ p->suid) && (currentp->uid ^ p->uid) && !suser())
	return -EPERM;
    if ((sig == SIGKILL) || (sig == SIGCONT)) {
	if (p->state == TASK_STOPPED)
	    wake_up_process(p);
	p->signal &= ~((1 << (SIGSTOP - 1)) | (1 << (SIGTSTP - 1))
#ifndef SMALLSIG
		       | (1 << (SIGTTIN - 1)) | (1 << (SIGTTOU - 1))
#endif
	    );
    }
    if (sig == SIGSTOP || sig == SIGTSTP
#ifndef SMALLSIG
	|| sig == SIGTTIN || sig == SIGTTOU
#endif
	)
	p->signal &= ~(1 << (SIGCONT - 1));
    /* Actually generate the signal */
    generate(sig, p);
    return 0;
}

int kill_pg(pid_t pgrp, sig_t sig, int priv)
{
    register struct task_struct *p;
    int err = -ESRCH;

    debug1("SIGNAL: Killing pg %d\n", pgrp);
    for_each_task(p)
	if (p->pgrp == pgrp)
	    err = send_sig(sig, p, priv);
    return err;
}

int kill_process(pid_t pid, sig_t sig, int priv)
{
    register struct task_struct *p;

    debug2("SIGNAL: Killing PID %d with sig %d.\n", pid, sig);
    for_each_task(p)
	if (p->pid == pid)
	    return send_sig(sig, p, 0);
    return -ESRCH;
}

int sys_kill(pid_t pid, sig_t sig)
{
    register __ptask pcurrent = current;
    register struct task_struct *p;
    int count, err, retval;

    if (((unsigned int)(sig - 1)) > (NSIG-1))
	return -EINVAL;
    if (!pid)
	return kill_pg(pcurrent->pgrp, sig, 0);

    count = retval = 0;

    if (pid == -1) {
	for_each_task(p)
	    if (p->pid > 1 && p != pcurrent) {
		count++;
		if ((err = send_sig(sig, p, 0)) != -EPERM)
		    retval = err;
	    }
	return (count ? retval : -ESRCH);
    }
    if (pid < 0)
	return (kill_pg(-pid, sig, 0));
    return kill_process(pid, sig, 0);
}

int sys_signal(int signr, __sighandler_t handler)
{
    debug2("SIGNAL: Registering action %x for signal %d.\n", handler, signr);
    if ( (((unsigned int)(--signr)) > (NSIG-1))
	 || signr == (SIGKILL-1) || signr == (SIGSTOP-1))
	return -EINVAL;
    current->sig.action[signr].sa_handler = handler;
    return 0;
}
