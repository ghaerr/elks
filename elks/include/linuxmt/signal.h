#ifndef LX86_LINUXMT_SIGNAL_H
#define LX86_LINUXMT_SIGNAL_H

/* The following signals mean nothing under ELKS currently:
 *
 * SIGBUS SIGTRAP SIGIOT SIGEMT SIGSYS SIGSTKFLT SIGPOLL
 * SIGCPU SIGPROF SIGPWR SIGILL SIGFPE
 * 
 * So we can have much tighter signal code if we have 16 bit signal
 * mask by losing all these unused signals.
 */
#define SMALLSIG

#ifdef SMALLSIG

typedef unsigned short sigset_t;	/* at least 16 bits */

#define _NSIG             16
#define NSIG		_NSIG

#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGWINCH	 4
#define SIGSTOP		 5
#define SIGABRT		 6
#define SIGTSTP		 7
#define SIGCONT		 8
#define SIGKILL		 9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGCHLD		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15

#else

typedef unsigned long sigset_t;	/* at least 32 bits */

#define _NSIG             32
#define NSIG		_NSIG

#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGBUS		 7
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGSTKFLT	16
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22
#define SIGURG		23
#define SIGXCPU		24
#define SIGXFSZ		25
#define SIGVTALRM	26
#define SIGPROF		27
#define SIGWINCH	28
#define SIGIO		29
#define SIGPOLL		SIGIO
#define SIGPWR		30
#define	SIGUNUSED	31

#endif

typedef int sig_t;

/*
 * sa_flags values: SA_STACK is not supported
 * SA_INTERRUPT is a no-op, but left due to historical reasons. Use the
 * SA_RESTART flag to get restarting signals (which were the default long ago)
 */

#define SA_NOCLDSTOP	1
#define SA_STACK	0x08000000
#define SA_RESTART	0x10000000
#define SA_INTERRUPT	0x20000000
#define SA_NOMASK	0x40000000
#define SA_ONESHOT	0x80000000

#ifdef __KERNEL__
/*
 * These values of sa_flags are used only by the kernel as part of the
 * irq handling routines.
 *
 * SA_INTERRUPT is also used by the irq handling routines.
 */
#define SA_PROBE SA_ONESHOT
#define SA_SAMPLE_RANDOM SA_RESTART
#endif

#define SIG_BLOCK          0	/* for blocking signals */
#define SIG_UNBLOCK        1	/* for unblocking signals */
#define SIG_SETMASK        2	/* for setting the signal mask */

/* Type of a signal handler.  */
typedef void (*__sighandler_t) ();

#define SIG_DFL	((__sighandler_t) 0)	/* default signal handling */
#define SIG_IGN	((__sighandler_t) 1)	/* ignore signal */
#define SIG_ERR	((__sighandler_t) -1)	/* error return from signal */

struct sigaction {
    __sighandler_t sa_handler;
/*	sigset_t sa_mask; */
/*	unsigned long sa_flags; */
/*	void (*sa_restorer)(); */
};

#endif
