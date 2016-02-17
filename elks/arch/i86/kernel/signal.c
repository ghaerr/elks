/*
 *  linux/arch/i386/kernel/signal.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Modified for ELKS 1998-1999 Al Riddoch
 */

#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
#include <linuxmt/kernel.h>
#include <linuxmt/signal.h>
#include <linuxmt/errno.h>
#include <linuxmt/wait.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>

int do_signal(void)
{
    register __ptask currentp = current;
    register __sighandler_t *sah;
    unsigned signr;

    while ((currentp->signal &= ((1L << NSIG) - 1))) {
	signr = find_first_non_zero_bit((void *)(&currentp->signal), NSIG);
        clear_bit(signr, &currentp->signal);

	debug2("Process %d has signal %d.\n", currentp->pid, signr);
	sah = &currentp->sig.action[signr].sa_handler;
	signr++;
	if (*sah == SIG_IGN) {
	    debug("Ignore\n");
	    continue;
	}
	if (*sah == SIG_DFL) {
	    debug("Default\n");
	    if (currentp->pid == 1)
		continue;
	    switch (signr) {
	    case SIGCHLD:
	    case SIGCONT:
	    case SIGWINCH:
		continue;

	    case SIGSTOP:
	    case SIGTSTP:
#ifndef SMALLSIG
	    case SIGTTIN:
	    case SIGTTOU:
#endif

		currentp->state = TASK_STOPPED;
		/* Let the parent know */
		currentp->p_parent->child_lastend = currentp->pid;
		currentp->p_parent->lastend_status = (int) signr;
		schedule();
		continue;
#if 0
	    case SIGABRT:
#ifndef SMALLSIG
	    case SIGFPE:
	    case SIGILL:
#endif
	    case SIGQUIT:
	    case SIGSEGV:
#ifndef SMALLSIG
	    case SIGTRAP:
#endif
#endif
		/* This is where we dump the core, which we must do */
	    default:
		do_exit((int) signr);
	    }
	}
	debug1("Setting up return stack for sig handler %x.\n", sa->sa_handler);
	debug1("Stack at %x\n", currentp->t_regs.sp);
	arch_setup_sighandler_stack(currentp, *sah, signr);
	debug1("Stack at %x\n", currentp->t_regs.sp);
	*sah = SIG_DFL;
        currentp->signal = 0;

	return 1;
    }

    return 0;
}

