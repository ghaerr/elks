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
    register struct sigaction *sa = &(p->sig.action[sig - 1]);
    sigset_t mask = 1 << (sig - 1);

    if (sa->sa_handler == SIG_IGN)
	return;
    if ((sa->sa_handler == SIG_DFL) &&
	(sig == SIGCONT || sig == SIGCHLD || sig == SIGWINCH
#ifndef SMALLSIG
	 || sig == SIGURG
#endif
	))
	return;
    printd_sig1("Generating sig %d.\n", sig);
    p->signal |= mask;
    if ((p->state == TASK_INTERRUPTIBLE) /* && (p->signal & ~p->blocked) */ ) {
	printd_sig("and wakig up\n");
	wake_up_process(p);
    }
}

int send_sig(pid_t sig, register struct task_struct *p, int priv)
{
    register __ptask currentp = current;
    printd_sig1("Killing with sig %d.\n", sig);
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

    printd_sig1("Killing pg %d\n", pgrp);
    for_each_task(p)
	if (p->pgrp == pgrp)
	    err = send_sig(sig, p, priv);
    return err;
}

int kill_process(pid_t pid, sig_t sig, int priv)
{
    register struct task_struct *p;

    printd_sig2("Killing PID %d with sig %d.\n", pid, sig);
    for_each_task(p)
	if (p->pid == pid)
	    return send_sig(sig, p, 0);
    return -ESRCH;
}

int sys_kill(pid_t pid, sig_t sig)
{
    register struct task_struct *p;
    int count = 0, err, retval = 0;

    if ((sig < 1) || (sig > NSIG))
	return -EINVAL;
    if (!pid)
	return kill_pg(current->pgrp, sig, 0);
    if (pid == -1) {
	for_each_task(p)
	    if (p->pid > 1 && p != current) {
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
    struct sigaction *sa;

    printd_sig2("Registering action %x for signal %d.\n", handler, signr);
    if (signr < 1 || signr > NSIG || signr == SIGKILL || signr == SIGSTOP)
	return -EINVAL;
    sa = &current->sig.action[signr - 1];
    sa->sa_handler = handler;
    return 0;
}
