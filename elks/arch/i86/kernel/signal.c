/*
 *  linux/arch/i386/kernel/signal.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
#include <linuxmt/kernel.h>
#include <linuxmt/signal.h>
#include <linuxmt/errno.h>
#include <linuxmt/wait.h>

#include <arch/segment.h>

#define _S(nr) (1<<((nr)-1))

#define put_user(val,ptr) pokew(current->t_regs.ds,ptr,val)

#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

int sys_waitpid();
int do_signal();

/*
 * atomically swap in the new signal mask, and wait for a signal.
 */
#if 0
int sys_sigsuspend(restart,oldmask,set)
int restart;
unsigned long oldmask;
unsigned long set;
{
	unsigned long mask;
	struct pt_regs * regs = (struct pt_regs *) &restart;

	mask = current->blocked;
	current->blocked = set & _BLOCKABLE;
#if 0	
	regs->eax = -EINTR;
#endif	
	while (1) {
		current->state = TASK_INTERRUPTIBLE;
		schedule();
		if (do_signal(mask,regs))
			return -EINTR;
	}
}
#endif

/*
 * This sets regs->esp even though we don't actually use sigstacks yet..
 */

int sys_sigreturn(__unused)
unsigned long __unused;
{
	/* FIXME */
	return -EINVAL;
}

/*
 * Set up a signal frame.
 * FIXME
 */

#if 0
static void setup_frame(sa,regs,signr,oldmask)
struct sigaction * sa;
struct pt_regs * regs;
int signr;
unsigned long oldmask;
{
	unsigned long * frame;

	frame = (unsigned long *) regs->esp;
	if (regs->ss != USER_DS && sa->sa_restorer)
		frame = (unsigned long *) sa->sa_restorer;
	frame -= 64;
	if (verify_area(VERIFY_WRITE,frame,64*4))
		do_exit(SIGSEGV);

/* set up the "normal" stack seen by the signal handler (iBCS2) */
#define __CODE ((unsigned long)(frame+24))
#define CODE(x) ((unsigned long *) ((x)+__CODE))
	put_user(__CODE,frame);
	if (current->exec_domain && current->exec_domain->signal_invmap)
		put_user(current->exec_domain->signal_invmap[signr], frame+1);
	else
		put_user(signr, frame+1);
	put_user(regs->gs, frame+2);
	put_user(regs->fs, frame+3);
	put_user(regs->es, frame+4);
	put_user(regs->ds, frame+5);
	put_user(regs->edi, frame+6);
	put_user(regs->esi, frame+7);
	put_user(regs->ebp, frame+8);
	put_user(regs->esp, frame+9);
	put_user(regs->ebx, frame+10);
	put_user(regs->edx, frame+11);
	put_user(regs->ecx, frame+12);
	put_user(regs->eax, frame+13);
	put_user(current->tss.trap_no, frame+14);
	put_user(current->tss.error_code, frame+15);
	put_user(regs->eip, frame+16);
	put_user(regs->cs, frame+17);
	put_user(regs->eflags, frame+18);
	put_user(regs->esp, frame+19);
	put_user(regs->ss, frame+20);
	put_user(save_i387((struct _fpstate *)(frame+32)),frame+21);
/* non-iBCS2 extensions.. */
	put_user(oldmask, frame+22);
	put_user(current->tss.cr2, frame+23);
/* set up the return code... */
	put_user(0x0000b858, CODE(0));	/* popl %eax ; movl $,%eax */
	put_user(0x80cd0000, CODE(4));	/* int $0x80 */
	put_user(__NR_sigreturn, CODE(2));
#undef __CODE
#undef CODE

	/* Set up registers for signal handler */
	regs->esp = (unsigned long) frame;
	regs->eip = (unsigned long) sa->sa_handler;
	regs->cs = USER_CS; regs->ss = USER_DS;
	regs->ds = USER_DS; regs->es = USER_DS;
	regs->gs = USER_DS; regs->fs = USER_DS;
	regs->eflags &= ~TF_MASK;
}
#endif	

/*
 * OK, we're invoking a handler
 */	

#if 0
static void handle_signal(signr,sa,oldmask,regs)
unsigned long signr;
struct sigaction *sa;
unsigned long oldmask;
struct pt_regs * regs;
{
	/* are we from a system call? */
	if (regs->orig_eax >= 0) {
		/* If so, check system call restarting.. */
		switch (regs->eax) {
			case -ERESTARTNOHAND:
				regs->eax = -EINTR;
				break;

			case -ERESTARTSYS:
				if (!(sa->sa_flags & SA_RESTART)) {
					regs->eax = -EINTR;
					break;
				}
			/* fallthrough */
			case -ERESTARTNOINTR:
				regs->eax = regs->orig_eax;
				regs->eip -= 2;
		}
	}

	/* set up the stack frame */
	setup_frame(sa, regs, signr, oldmask);

	if (sa->sa_flags & SA_ONESHOT)
		sa->sa_handler = NULL;
	current->blocked |= sa->sa_mask;
}
#endif	


int do_signal()
{
	struct sigaction * sa;
	unsigned signr;

	while (current->signal) {
		signr = find_first_non_zero_bit(&current->signal, 32);
		if (signr == 32) {
			printk("THIS SHOULD NEVER HAPPEN\n");
		}
		sa = current->sig.action + signr;
		signr++;
		if (sa->sa_handler == SIG_IGN) {
			if (signr != SIGCHLD)
				continue;
			while (sys_wait4(-1,NULL,WNOHANG) > 0);
			continue;
		}
		if (sa->sa_handler == SIG_DFL) {
			if (current->pid == 1)
				continue;
			switch (signr) {
			case SIGCONT: case SIGCHLD: case SIGWINCH:
				continue;
			case SIGSTOP: case SIGTSTP: case SIGTTIN: case SIGTTOU:
				current->state = TASK_STOPPED;
			/*	current->exit_code =  signr; */
			/* Let the parent know */
				current->p_parent->child_lastend = current->pid;
				current->p_parent->lastend_status = signr;
				schedule();
				continue;
/*			case SIGQUIT: case SIGILL: case SIGTRAP:
			case SIGABRT: case SIGFPE: case SIGSEGV:
		/* This is where w dump the core */
			default:
				sys_exit(signr);
			}
		}
/*		handle_signal(signr, sa); */
		return 1;
	}
	return 0;
}
	

#if 0
/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 *
 * Note that we go through the signals twice: once to check the signals that
 * the kernel can handle, and then we build all the user-level signal handling
 * stack-frames in one go after that.
 */
int do_signal(oldmask,regs)
unsigned long oldmask;
struct pt_regs * regs;
{
	unsigned long mask = ~current->blocked;
	unsigned long signr;
	struct sigaction * sa;

	while ((signr = current->signal & mask)) {
		/*
		 *	This stops gcc flipping out. Otherwise the assembler
		 *	including volatiles for the inline function to get
		 *	current combined with this gets it confused.
		 */
	        struct task_struct *t=current;
		__asm__("bsf %3,%1\n\t"
			"btrl %1,%0"
			:"=m" (t->signal),"=r" (signr)
			:"0" (t->signal), "1" (signr));
		sa = current->sig->action + signr;
		signr++;
		if ((current->flags & PF_PTRACED) && signr != SIGKILL) {
			current->exit_code = signr;
			current->state = TASK_STOPPED;
			notify_parent(current);
			schedule();
			if (!(signr = current->exit_code))
				continue;
			current->exit_code = 0;
			if (signr == SIGSTOP)
				continue;
			if (_S(signr) & current->blocked) {
				current->signal |= _S(signr);
				continue;
			}
			sa = current->sig->action + signr - 1;
		}
		if (sa->sa_handler == SIG_IGN) {
			if (signr != SIGCHLD)
				continue;
			/* check for SIGCHLD: it's special */
			while (sys_waitpid(-1,NULL,WNOHANG) > 0)
				/* nothing */;
			continue;
		}
		if (sa->sa_handler == SIG_DFL) {
			if (current->pid == 1)
				continue;
			switch (signr) {
			case SIGCONT: case SIGCHLD: case SIGWINCH:
				continue;

			case SIGSTOP: case SIGTSTP: case SIGTTIN: case SIGTTOU:
				if (current->flags & PF_PTRACED)
					continue;
				current->state = TASK_STOPPED;
				current->exit_code = signr;
				if (!(current->p_pptr->sig->action[SIGCHLD-1].sa_flags & 
						SA_NOCLDSTOP))
					notify_parent(current);
				schedule();
				continue;

			case SIGQUIT: case SIGILL: case SIGTRAP:
			case SIGABRT: case SIGFPE: case SIGSEGV:
				if (current->binfmt && current->binfmt->core_dump) {
					if (current->binfmt->core_dump(signr, regs))
						signr |= 0x80;
				}
				/* fall through */
			default:
				current->signal |= _S(signr & 0x7f);
				current->flags |= PF_SIGNALED;
				do_exit(signr);
			}
		}
		handle_signal(signr, sa, oldmask, regs);
		return 1;
	}

	/* Did we come from a system call? */
	if (regs->orig_eax >= 0) {
		/* Restart the system call - no handlers present */
		if (regs->eax == -ERESTARTNOHAND ||
		    regs->eax == -ERESTARTSYS ||
		    regs->eax == -ERESTARTNOINTR) {
			regs->eax = regs->orig_eax;
			regs->eip -= 2;
		}
	}
	return 0;
}
#endif	
