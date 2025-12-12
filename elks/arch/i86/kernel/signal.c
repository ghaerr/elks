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
#include <arch/irq.h>

void do_signal(void)
{
    register __sigdisposition_t *sd;
    register __kern_sighandler_t sah;
    unsigned int signr;
    sigset_t mask;

    signr = 1;
    mask = (sigset_t)1;
    while (current->signal) {
	while(!(current->signal & mask)) {
	    signr++;
	    mask <<= 1;
	}
	current->signal ^= mask;

	debug_sig("SIGNAL(%P) do_signal sig %d\n", signr);
	sah = current->sig.handler;
	sd = &current->sig.action[signr - 1].sa_dispose;
	if (*sd == SIGDISP_DFL) {			/* Default */
	    if ((mask &					/* Default Ignore */
			(SM_SIGCONT | SM_SIGCHLD | SM_SIGWINCH | SM_SIGURG))
		|| (current->pid == 1 && signr != SIGKILL))
		continue;
	    else
            if (mask &					/* Default Stop */
			(SM_SIGSTOP | SM_SIGTSTP | SM_SIGTTIN | SM_SIGTTOU)) {
		debug_sig("SIGNAL(%P) stopped sig %d\n", signr);
		current->state = TASK_STOPPED;
		current->exit_status = (signr<<8)|0x7f;	/* Let the parent know */
		wake_up(&current->p_parent->child_wait);
		schedule();
		/* task continues here after SIGCONT */
		current->signal = 0;                    /* clear any SIGINT/SIGQUIT */
		return;
	    }
	    else {					/* Default Terminate */
#if UNUSED
		if (mask &
		      (SM_SIGQUIT|SM_SIGILL|SM_SIGABRT|SM_SIGFPE|SM_SIGSEGV|SM_SIGTRAP))
		    dump_core();
#endif
		debug_sig("SIGNAL(%P) do_exit sig %d\n", signr);
		do_exit(signr);
		/* no return */
	    }
	}
	else if (*sd != SIGDISP_IGN) {			/* Set handler */
	    debug_sig("SIGNAL(%P) calling handler %x:%x\n",
		_FP_SEG(sah), _FP_OFF(sah));
	    arch_setup_sighandler_stack(current, sah, signr);
	    *sd = SIGDISP_DFL;
	    clr_irq();		/* stop race between reset signal and return to user */
	    current->signal &= ~mask;
	    if (current->signal) {
		debug_sig("SIGNAL(%P) current mask %04x, next signal mask %04x\n",
		    mask, current->signal);
	    }
	    return;
	}
	else /* else (*sd == SIGDISP_IGN) Ignore */
	    debug_sig("SIGNAL(%P) sig %d ignored\n", signr);
    }
}

