/*
 *  linux/kernel/signal.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

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
	unsigned long mask = 1 << (sig-1);
	register struct sigaction * sa = sig + p->sig.action - 1;

/*	if (!(mask & p->blocked)) { */
		if (sa->sa_handler == SIG_IGN && sig != SIGCHLD)
			return;
		if ((sa->sa_handler == SIG_DFL) &&
		    (sig == SIGCONT || sig == SIGCHLD || 
		     sig == SIGWINCH || sig == SIGURG))
			return;
/*	} */
	printk("Generating sig %d.\n", sig);
	p->signal |= mask;
	if ((p->state == TASK_INTERRUPTIBLE) /* && (p->signal & ~p->blocked) */) {
		printk("and wakig up\n");
		wake_up_process(p);
	}
}

int send_sig(sig, p, priv)
int sig;
register struct task_struct * p;
int priv;
{
	register __ptask currentp = current;
	printk("Killing with sig %d.\n", sig);
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
		p->signal &= ~( (1<<(SIGSTOP-1)) | (1<<(SIGTSTP-1)) |
			(1<<(SIGTTIN-1)) | (1<<(SIGTTOU-1)) );
	}
	if (sig == SIGSTOP || sig == SIGTSTP || sig == SIGTTIN || sig == SIGTTOU)
                p->signal &= ~(1<<(SIGCONT-1));
        /* Actually generate the signal */
        generate(sig,p);
        return 0;
}



int sys_kill(pid, sig)
int pid;
int sig;
{
	register struct task_struct * p;

	printk("Killing PID %d with sig %d.\n", pid, sig);
	if ((sig < 1) || (sig > 32))
		return -EINVAL;
	for_each_task(p) {
		if (p->pid == pid)
			return send_sig(sig,p,0);
	}
	return -ESRCH;
}

