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
#include <linuxmt/memory.h>

#include <arch/segment.h>

int do_signal(void)
{
    register __ptask currentp = current;
    register __sigdisposition_t *sd;
    register __kern_sighandler_t sah;
    unsigned int signr;
    sigset_t mask;

    signr = 1;
    mask = (sigset_t)1;
    while (currentp->signal) {
	while(!(currentp->signal & mask)) {
	    signr++;
	    mask <<= 1;
	}
	currentp->signal ^= mask;

	debug_sig("SIGNAL process signal %d pid %P\n", signr);
	sah = currentp->sig.handler;
	sd = &currentp->sig.action[signr - 1].sa_dispose;
	if (*sd == SIGDISP_DFL) {			/* Default */
	    if ((mask &					/* Default Ignore */
			(SM_SIGCONT | SM_SIGCHLD | SM_SIGWINCH | SM_SIGURG))
		|| (currentp->pid == 1 && signr != SIGKILL))
		continue;
	    else if (mask &				/* Default Stop */
			(SM_SIGSTOP | SM_SIGTSTP | SM_SIGTTIN | SM_SIGTTOU)) {
		debug_sig("SIGNAL pid %P stopped\n");
		currentp->state = TASK_STOPPED;
		/* Let the parent know */
		currentp->exit_status = signr;
		schedule();
	    }
	    else {					/* Default Core or Terminate */
#if UNUSED
		if (mask &				/* Default Core */
		      (SM_SIGQUIT|SM_SIGILL|SM_SIGABRT|SM_SIGFPE|SM_SIGSEGV|SM_SIGTRAP))
		    dump_core();
#endif
		debug_sig("SIGNAL terminating pid %P\n");
		do_exit(signr);				/* Default Terminate */
	    }
	}
	else if (*sd != SIGDISP_IGN) {			/* Set handler */
	    debug_sig("SIGNAL setup return stack for handler %x:%x\n",
		      _FP_SEG(sah), _FP_OFF(sah));
	    arch_setup_sighandler_stack(currentp, sah, signr);
	    *sd = SIGDISP_DFL;
	    debug_sig("SIGNAL reset pending signals\n");
	    clr_irq();		/* stop race between reset signal and return to user */
	    currentp->signal &= ~mask;
	    if (currentp->signal)
		printk("SIGNAL(%P) processing mask %04x, additional signal w/mask %04x\n",
		    mask, currentp->signal);

	    return 1;
	}
	else /* else (*sd == SIGDISP_IGN) Ignore */
	    debug_sig("SIGNAL signal %d ignored pid %P\n", signr);
    }
    return 0;
}

