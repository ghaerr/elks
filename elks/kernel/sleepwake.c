#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <linuxmt/wait.h>
#include <linuxmt/trace.h>
#include <linuxmt/debug.h>

#include <arch/irq.h>
#include <arch/segment.h>

/*
 *  Wait queue functionality for Linux ELKS. Taken from sched.c/h of
 *  its big brother..
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *  Elks tweaks  (C) 1995 Alan Cox.
 *
 *  Copyright (C) 2000 Alan Cox.
 *  Rewrote the entire queue functionality to reflect ELKS actual needs
 *  not Linux needs
 */

/*
 * Simple, race-safe versions of sleep/wait functions. Greg Haerr Aug 2020.
 * Only required when wait queue used or condition check changed by hw interrupt.
 *
 * Old:
 *      if (!ready())
 *          interruptible_sleep_on(&waitq);
 * New:
 *      prepare_to_wait_interruptible(&waitq);
 *      if (!ready())
 *          do_wait();
 *      finish_wait(&waitq);
 */

void prepare_to_wait_interruptible(struct wait_queue *p)
{
    current->state = TASK_INTERRUPTIBLE;
    wait_set(p);
}

void prepare_to_wait(struct wait_queue *p)
{
    current->state = TASK_UNINTERRUPTIBLE;
    wait_set(p);
}

void do_wait(void)
{
    debug_sched("sleep: %P waitq %04x\n", current->waitpt);
    schedule();
}

void finish_wait(struct wait_queue *p)
{
    current->state = TASK_RUNNING;
    wait_clear(p);
}

/**********************************/

void wait_set(struct wait_queue *p)
{
#ifdef CHECK_SCHED
    if (current->waitpt) panic("SCHED: wait_set double wait");
#endif
    current->waitpt = p;
}

void wait_clear(struct wait_queue *p)
{
#ifdef CHECK_SCHED
    if (current->waitpt != p) panic("SCHED: wait_clear wrong waitpt");
#endif
    current->waitpt = NULL;
}

static void __sleep_on(register struct wait_queue *p, int state)
{
#ifdef CHECK_SCHED
    if (current == &task[0]) panic("SCHED: trying to sleep idle task on %x", p);
#endif
    debug_sched("sleep: %P waitq %04x\n", p);
    current->state = state;
    wait_set(p);
    schedule();
    wait_clear(p);
}

void sleep_on(struct wait_queue *p)
{
    __sleep_on(p, TASK_UNINTERRUPTIBLE);
}

void interruptible_sleep_on(struct wait_queue *p)
{
    __sleep_on(p, TASK_INTERRUPTIBLE);
}

/*
 * Wake up a process. Put it on the run-queue if it's not
 * already there.  The "current" process is always on the
 * run-queue (except when the actual re-schedule is in
 * progress), and as such you're allowed to do the simpler
 * "current->state = TASK_RUNNING" to mark yourself runnable
 * without the overhead of this.
 */

void wake_up_process(register struct task_struct *p)
{
    flag_t flags;

    debug_sched("wakeup: %d\n", p->pid);
    save_flags(flags);
    clr_irq();
    p->state = TASK_RUNNING;
    if (!p->next_run)
        add_to_runqueue(p);
    restore_flags(flags);
}

/*
 * wake_up doesn't wake up stopped processes - they have to be awakened
 * with signals or similar.
 *
 * Note that this doesn't need clr_irq / set_irq pairs: interrupts may not
 * change the wait-queue structures directly, but only call wake_up() to wake
 * a process. The process itself must remove the queue once it has woken.
 */

void _wake_up(register struct wait_queue *q, int it)
{
    register struct task_struct *p;

    for_each_task(p) {
        if (p->state == TASK_UNUSED)
                continue;
        if ((p->waitpt == q) || ((p->waitpt == &select_queue) && select_poll (p, q)))
            if (p->state == TASK_INTERRUPTIBLE ||
                (it && p->state == TASK_UNINTERRUPTIBLE)) {
                wake_up_process(p);
            }
    }
}

/*
 *  Semaphores. These are not IRQ safe nor needed to be so for ELKS
 */

void up(register sem_t *s)
{
    if (++(*s) == 0)            /* Gone non-negative */
        wake_up((void *) s);

    if (*s != 0) debug_net("kernel: sem up %x FAIL %d\n", s, *s);
}

void down(register sem_t *s)
{
    /* Wait for the semaphore */
    while (*s < 0)
        sleep_on((void *) s);

    /* Take it */
    --(*s);

    if (*s != -1) debug_net("kernel: sem down %x FAIL %d\n", s, *s);
}
