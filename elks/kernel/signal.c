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

static void generate(sig_t sig, sigset_t msksig, register struct task_struct *p)
{
    register __sigdisposition_t sd;

    sd = p->sig.action[sig - 1].sa_dispose;
    if ((sd == SIGDISP_IGN) ||
       ((sd == SIGDISP_DFL) &&
        (msksig & (SM_SIGCONT | SM_SIGCHLD | SM_SIGWINCH | SM_SIGURG)))) {
        if (!(msksig & SM_SIGCHLD))
            debug_sig("SIGNAL ignoring %d pid %d\n", sig, p->pid);
        return;
    }
    debug_sig("SIGNAL gen_sig %d mask %x action %x:%x pid %d\n", sig, msksig,
              _FP_SEG(p->sig.handler), _FP_OFF(p->sig.handler), p->pid);
    p->signal |= msksig;
    if ((p->state == TASK_INTERRUPTIBLE) /* && (p->signal & ~p->blocked) */ ) {
        debug_sig("SIGNAL wakeup pid %d\n", p->pid);
        wake_up_process(p);
    }
}

int send_sig(sig_t sig, register struct task_struct *p, int priv)
{
    sigset_t msksig;

    if (sig != SIGCHLD) debug_sig("SIGNAL send_sig %d pid %d\n", sig, p->pid);
    if (!priv && ((sig != SIGCONT) || (current->session != p->session)) &&
        (current->euid ^ p->suid) && (current->euid ^ p->uid) &&
        (current->uid ^ p->suid) && (current->uid ^ p->uid) && !suser())
        return -EPERM;
    msksig = (((sigset_t)1) << (sig - 1));
    if (msksig & (SM_SIGKILL | SM_SIGCONT)) {
        if (p->state == TASK_STOPPED)
            wake_up_process(p);
        p->signal &= ~(SM_SIGSTOP | SM_SIGTSTP | SM_SIGTTIN | SM_SIGTTOU);
    }
    if (msksig & (SM_SIGSTOP | SM_SIGTSTP | SM_SIGTTIN | SM_SIGTTOU))
        p->signal &= ~SM_SIGCONT;
    /* Actually generate the signal */
    generate(sig, msksig, p);
    return 0;
}

int kill_pg(pid_t pgrp, sig_t sig, int priv)
{
    register struct task_struct *p;
    int err = -ESRCH;

    if (!pgrp) {
        debug_sig("SIGNAL kill_pg 0 ignored\n");
        return 0;
    }
    debug_sig("SIGNAL kill_pg sig %d pgrp %d\n", sig, pgrp);
    for_each_task(p) {
        if (p->pgrp == pgrp) {
                if (p->state < TASK_ZOMBIE)
                    err = send_sig(sig, p, priv);
                else if (p->state != TASK_UNUSED)
                    debug_sig("SIGNAL skip kill_pg pgrp %d pid %d state %d\n",
                                        pgrp, p->pid, p->state);
        }
    }
    return err;
}

int kill_process(pid_t pid, sig_t sig, int priv)
{
    register struct task_struct *p;

    debug_sig("SIGNAL kill_proc sig %d pid %d\n", sig, pid);
    for_each_task(p)
        if (p->pid == pid && p->state < TASK_ZOMBIE)
            return send_sig(sig, p, 0);
    return -ESRCH;
}

void kill_all(sig_t sig)
{
    register struct task_struct *p;

    debug_sig("SIGNAL kill_all %d\n", sig);
    for_each_task(p)
        if (p->state < TASK_ZOMBIE)
            send_sig(sig, p, 0);
}

int sys_kill(pid_t pid, sig_t sig)
{
    register struct task_struct *p;
    int count, err, retval;

    debug_sig("SIGNAL sys_kill %d, %d pid %P\n", pid, sig);
    if ((unsigned int)(sig - 1) > (NSIG-1))
        return -EINVAL;

    count = retval = 0;
    if (pid == (pid_t)-1) {
        for_each_task(p)
            if (p->pid > 1 && p != current && p->state < TASK_ZOMBIE) {
                count++;
                if ((err = send_sig(sig, p, 0)) != -EPERM)
                    retval = err;
            }
        return (count ? retval : -ESRCH);
    }
    if (pid < 1)
        return kill_pg((!pid ? current->pgrp : -pid), sig, 0);
    return kill_process(pid, sig, 0);
}

int sys_signal(int signr, __kern_sighandler_t handler)
{
    int i;
    struct segment *s;

    debug_sig("SIGNAL sys_signal %d action %x:%x pid %P\n", signr,
              _FP_SEG(handler), _FP_OFF(handler));
    if (((unsigned int)signr > NSIG) || signr == SIGKILL || signr == SIGSTOP)
        return -EINVAL;
    if (handler == KERN_SIG_DFL)
        current->sig.action[signr - 1].sa_dispose = SIGDISP_DFL;
    else if (handler == KERN_SIG_IGN)
        current->sig.action[signr - 1].sa_dispose = SIGDISP_IGN;
    else {
        for (i = 0; i < MAX_SEGS; i++) {
            s = current->mm[i];
            if (!s || (s->flags & SEG_FLAG_TYPE) != SEG_FLAG_CSEG)
                continue;
            if (_FP_SEG(handler) < s->base || _FP_SEG(handler) >= s->base + s->size) {
                printk("SIGNAL sys_signal supplied handler is bad\n");
                debug_sig("SIGNAL sys_signal cs not in [%x, %x)\n",
                    s->base, s->base + s->size);
                return -EINVAL;
            }
        }
        current->sig.handler = handler;
        current->sig.action[signr - 1].sa_dispose = SIGDISP_CUSTOM;
    }
    return 0;
}
