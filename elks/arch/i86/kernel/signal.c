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
    register struct sigaction *sa;
    unsigned signr;

    while (currentp->signal) {
	signr = find_first_non_zero_bit(&currentp->signal, NSIG);
	if (signr == NSIG)
	    panic("No signal set!\n");

	debug2("Process %d has signal %d.\n", currentp->pid, signr);
	sa = &currentp->sig.action[signr];
	signr++;
	if (sa->sa_handler == SIG_IGN) {
	    debug("Ignore\n");
	    continue;
	}
	if (sa->sa_handler == SIG_DFL) {
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
		currentp->p_parent->lastend_status = signr;
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
		do_exit(signr);
	    }
	}
	debug1("Setting up return stack for sig handler %x.\n", sa->sa_handler);
	debug1("Stack at %x\n", current->t_regs.sp);
	arch_setup_sighandler_stack(current, sa->sa_handler, signr);
	debug1("Stack at %x\n", current->t_regs.sp);
	sa->sa_handler = SIG_DFL;

	return 1;
    }

    return 0;
}
