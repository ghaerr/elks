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

#define _S(nr) (1<<((nr)-1))

#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

#define put_user(val,ptr) pokew(current->t_regs.ds,ptr,val)
#define get_user(ptr) peekw(current->t_regs.ds,ptr)

static void generate(sig, p)
int sig;
register struct task_struct * p;
{
	sigset_t mask = 1 << (sig-1);
	register struct sigaction * sa = &(p->sig.action[sig - 1]);

/*	if (!(mask & p->blocked)) { */
		if (sa->sa_handler == SIG_IGN && sig != SIGCHLD)
			return;
		if ((sa->sa_handler == SIG_DFL) &&
		    (sig == SIGCONT || sig == SIGCHLD || sig == SIGWINCH
#ifndef SMALLSIG
			|| sig == SIGURG
#endif
			))
			return;
/*	} */
	printd_sig1("Generating sig %d.\n", sig);
	p->signal |= mask;
	if ((p->state == TASK_INTERRUPTIBLE) /* && (p->signal & ~p->blocked) */) {
		printd_sig("and wakig up\n");
		wake_up_process(p);
	}
}

int send_sig(sig, p, priv)
pid_t sig;
register struct task_struct * p;
int priv;
{
	register __ptask currentp = current;
	printd_sig1("Killing with sig %d.\n", sig);
	if (!priv && ((sig != SIGCONT) || (currentp->session != p->session)) &&
	    (currentp->euid ^ p->suid) && (currentp->euid ^ p->uid) &&
	    (currentp->uid ^ p->suid) && (currentp->uid ^ p->uid) &&
	    !suser())
		return -EPERM;
/*	if (!p->sig)
		return 0; */ /* Does not seem to be valid in ELKS */
	if ((sig == SIGKILL) || (sig == SIGCONT)) {
		if (p->state == TASK_STOPPED)
			wake_up_process(p);
/*		p->exit_code = 0; */
		p->signal &= ~( (1<<(SIGSTOP-1)) | (1<<(SIGTSTP-1))
#ifndef SMALLSIG
			| (1<<(SIGTTIN-1)) | (1<<(SIGTTOU-1))
#endif
			);
	}
#ifdef SMALLSIG
	if (sig == SIGSTOP || sig == SIGTSTP) {
#else
	if (sig == SIGSTOP || sig == SIGTSTP || sig == SIGTTIN || sig == SIGTTOU ) {
#endif
                p->signal &= ~(1<<(SIGCONT-1));
	}
        /* Actually generate the signal */
        generate(sig,p);
        return 0;
}

int kill_pg(pgrp, sig, priv)
pid_t pgrp;
int sig;
int priv;
{
	register struct task_struct * p;
	int err = -ESRCH;

	printd_sig1("Killing pg %d\n", pgrp);
	for_each_task(p) {
		if (p->pgrp == pgrp) {
			err = send_sig(sig,p,priv);
		}
	}
	return err;
}

int kill_process(pid, sig, priv)
pid_t pid;
int sig;
int priv;
{
	register struct task_struct * p;

	printd_sig2("Killing PID %d with sig %d.\n", pid, sig);
	for_each_task(p) {
		if (p->pid == pid)
			return send_sig(sig,p,0);
	}
	return -ESRCH;
}
	

int sys_kill(pid, sig)
pid_t pid;
int sig;
{

	if ((sig < 1) || (sig > NSIG))
		return -EINVAL;
	return kill_process(pid, sig, 0);
}

int sys_signal(signr, handler)
int signr;
__sighandler_t handler;
{
	struct sigaction * sa;

	printd_sig2("Registering action %x for signal %d.\n", handler, signr);
	if (signr<1 || signr>NSIG || signr==SIGKILL || signr==SIGSTOP) {
		return -EINVAL;
	}
	sa = &current->sig.action[signr - 1];
	sa->sa_handler = handler;
	
	return 0;
}
