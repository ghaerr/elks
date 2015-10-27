#include <arch/irq.h>
#include <arch/segment.h>

#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <linuxmt/wait.h>

/*
 *	Wait queue functionality for Linux ELKS. Taken from sched.c/h of
 *	its big brother..
 *
 *	Copyright (C) 1991, 1992  Linus Torvalds
 *	Elks tweaks  (C) 1995 Alan Cox.
 *
 *	Copyright (C) 2000 Alan Cox.
 *	Rewrote the entire queue functionality to reflect ELKS actual needs
 *	not Linux needs
 */

void wait_set(struct wait_queue *p)
{
    register __ptask pcurrent = current;

    if (pcurrent->waitpt)
	panic("double wait");
    pcurrent->waitpt = p;
}

void wait_clear(struct wait_queue *p)
{
    register __ptask pcurrent = current;

    if (pcurrent->waitpt != p)
	panic("wrong waitpt");
    pcurrent->waitpt = NULL;
}

static void __sleep_on(register struct wait_queue *p, __s16 state)
{
    register __ptask pcurrent = current;

    if (pcurrent == &task[0]) {
	printk("task[0] trying to sleep ");
	panic("from %x", (int)p);
    }
    pcurrent->state = state;
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
	save_flags(flags);
	clr_irq();
	p->state = TASK_RUNNING;
	if (!p->next_run){
	    add_to_runqueue(p);
	}
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

void _wake_up(struct wait_queue *q, unsigned short int it)
{
    register struct task_struct *p;
    unsigned short int phash;

    extern struct wait_queue select_poll;

    phash = ((unsigned short int)1) << (((unsigned short int)q >> 8) & 0x0F);

    for_each_task(p) {
	if ((p->waitpt == q)
	    || ((p->waitpt == &select_poll) && (p->pollhash & phash))
	    )
	    if (p->state == TASK_INTERRUPTIBLE ||
		(it && p->state == TASK_UNINTERRUPTIBLE)) {
		wake_up_process(p);
	    }
    }
}

/*
 *	Semaphores. These are not IRQ safe nor needed to be so for ELKS
 */

void up(register short int *s)
{
    if (++(*s) == 0)		/* Gone non-negative */
	wake_up((void *) s);
}

void down(register short int *s)
{
    /* Wait for the semaphore */
    while (*s < 0)
	sleep_on((void *) s);

    /* Take it */
    --(*s);
}
