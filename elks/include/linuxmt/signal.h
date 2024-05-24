#ifndef __LINUXMT_SIGNAL_H
#define __LINUXMT_SIGNAL_H

/* The following signals mean nothing under ELKS currently:
 *
 * SIGBUS SIGTRAP SIGIOT SIGEMT SIGSYS SIGSTKFLT SIGPOLL
 * SIGCPU SIGPROF SIGPWR SIGILL SIGFPE
 * 
 * So we can have much tighter signal code if we have 16 bit signal
 * mask by losing all these unused signals.
 */
#include <linuxmt/types.h>
#include <arch/cdefs.h>

#define __SMALLSIG		/* 16-bit sigset_t*/

#ifdef __SMALLSIG

typedef unsigned short sigset_t;	/* at least 16 bits */

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
#define SIGURG		16
#define SIGTTIN		0		/* FIXME not implemented*/
#define SIGUSR2		0		/* FIXME not implemented*/

#define _NSIG		16

#else

typedef unsigned long sigset_t;	/* at least 32 bits */

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

#define _NSIG		32

#endif

#define NSIG		_NSIG

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

#ifdef __SMALLSIG

#define SM_SIGHUP	(1 << (SIGHUP - 1))
#define SM_SIGINT	(1 << (SIGINT - 1))
#define SM_SIGQUIT	(1 << (SIGQUIT - 1))
#define SM_SIGWINCH	(1 << (SIGWINCH - 1))
#define SM_SIGSTOP	(1 << (SIGSTOP - 1))
#define SM_SIGABRT	(1 << (SIGABRT - 1))
#define SM_SIGTSTP	(1 << (SIGTSTP - 1))
#define SM_SIGCONT	(1 << (SIGCONT - 1))
#define SM_SIGKILL	(1 << (SIGKILL - 1))
#define SM_SIGUSR1	(1 << (SIGUSR1 - 1))
#define SM_SIGSEGV	(1 << (SIGSEGV - 1))
#define SM_SIGCHLD	(1 << (SIGCHLD - 1))
#define SM_SIGPIPE	(1 << (SIGPIPE - 1))
#define SM_SIGALRM	(1 << (SIGALRM - 1))
#define SM_SIGTERM	(1 << (SIGTERM - 1))
#define SM_SIGURG	(1 << (SIGURG - 1))

#define SM_SIGILL	0
#define SM_SIGFPE	0
#define SM_SIGTTIN	0
#define SM_SIGTTOU	0
#define SM_SIGTRAP	0

#else

#define SM_SIGHUP	(1L << (SIGHUP - 1))
#define SM_SIGINT	(1L << (SIGINT - 1))
#define SM_SIGQUIT	(1L << (SIGQUIT - 1))
#define SM_SIGILL	(1L << (SIGILL - 1))
#define SM_SIGTRAP	(1L << (SIGTRAP - 1))
#define SM_SIGABRT	(1L << (SIGABRT - 1))
#define SM_SIGIOT	(1L << (SIGIOT - 1))
#define SM_SIGBUS	(1L << (SIGBUS - 1))
#define SM_SIGFPE	(1L << (SIGFPE - 1))
#define SM_SIGKILL	(1L << (SIGKILL - 1))
#define SM_SIGUSR1	(1L << (SIGUSR1 - 1))
#define SM_SIGSEGV	(1L << (SIGSEGV - 1))
#define SM_SIGUSR2	(1L << (SIGUSR2 - 1))
#define SM_SIGPIPE	(1L << (SIGPIPE - 1))
#define SM_SIGALRM	(1L << (SIGALRM - 1))
#define SM_SIGTERM	(1L << (SIGTERM - 1))
#define SM_SIGSTKFLT	(1L << (SIGSTKFLT - 1))
#define SM_SIGCHLD	(1L << (SIGCHLD - 1))
#define SM_SIGCONT	(1L << (SIGCONT - 1))
#define SM_SIGSTOP	(1L << (SIGSTOP - 1))
#define SM_SIGTSTP	(1L << (SIGTSTP - 1))
#define SM_SIGTTIN	(1L << (SIGTTIN - 1))
#define SM_SIGTTOU	(1L << (SIGTTOU - 1))
#define SM_SIGURG	(1L << (SIGURG - 1))
#define SM_SIGXCPU	(1L << (SIGXCPU - 1))
#define SM_SIGXFSZ	(1L << (SIGXFSZ - 1))
#define SM_SIGVTALRM	(1L << (SIGVTALRM - 1))
#define SM_SIGPROF	(1L << (SIGPROF - 1))
#define SM_SIGWINCH	(1L << (SIGWINCH - 1))
#define SM_SIGIO	(1L << (SIGIO - 1))
#define SM_SIGPOLL	(1L << (SIGPOLL - 1))
#define SM_SIGPWR	(1L << (SIGPWR - 1))
#define	SM_SIGUNUSED	(1L << (SIGUNUSED - 1))

#endif

#endif /* __KERNEL__*/

#define SIG_BLOCK          0	/* for blocking signals */
#define SIG_UNBLOCK        1	/* for unblocking signals */
#define SIG_SETMASK        2	/* for setting the signal mask */

/* Type of a signal handler within userland.  */
typedef void (*sighandler_t)(int);
/* Type of a signal handler which interfaces with the kernel.  This is always
   a far function that uses the `stdcall' calling convention, even for a
   user program that is being compiled for a different calling convention.  */
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
typedef stdcall __far void (*__kern_sighandler_t)(int);
#pragma GCC diagnostic pop
#endif

#ifdef __WATCOMC__
typedef void stdcall (__far *__kern_sighandler_t)(int);
#endif

/*
 * Because this stuff can get pretty confusing:
 *   * SIG_DFL, SIG_IGN, and SIG_ERR are used within userland.  These may be
 *     16-bit near "pointers", or (for medium model programs) 32-bit far
 *     "pointers".
 *   * KERN_SIG_DFL and KERN_SIG_IGN are used by the userland <-> kernel
 *     interface.  These are always 32-bit.
 *   * SIGDISP_DFL, SIGDISP_IGN, and SIGDISP_CUSTOM are used internally by
 *     the kernel, to record the disposition of each particular type of
 *     signal --- whether to use the default handling, to ignore the signal,
 *     or to use the custom handler (which is the same throughout a single
 *     process).  As there are only 3 possible dispositions, the SIGDISP_*
 *     values can be 8-bit or smaller.  -- tkchia 20200512
 */

#define SIG_DFL	((sighandler_t) 0)	/* default signal handling */
#define SIG_IGN	((sighandler_t) 1)	/* ignore signal */
#define SIG_ERR	((sighandler_t) -1)	/* error return from signal */

typedef unsigned char __sigdisposition_t;

#ifdef __KERNEL__
#define KERN_SIG_DFL ((__kern_sighandler_t) 0L)  /* default signal handling */
#define KERN_SIG_IGN ((__kern_sighandler_t) 1L)  /* ignore signal */

#define SIGDISP_DFL	((__sigdisposition_t) 0)
#define SIGDISP_IGN	((__sigdisposition_t) 1)
#define SIGDISP_CUSTOM	((__sigdisposition_t) 2)
#endif

/*@end@*/

struct __kern_sigaction_struct {
    __sigdisposition_t sa_dispose;
#if UNUSED
    sigset_t sa_mask;
    unsigned long sa_flags;
    void (*sa_restorer)();
#endif
};

#ifdef __KERNEL__
struct task_struct;
extern int send_sig(sig_t,struct task_struct *,int);
extern void arch_setup_sighandler_stack(register struct task_struct *,
					__kern_sighandler_t,unsigned);
extern void ctrl_alt_del(void);
#endif /* __KERNEL__*/

#endif
