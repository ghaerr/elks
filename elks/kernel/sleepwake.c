#include <arch/irq.h>
#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/wait.h>

/*
 *	Wait queue functionality for Linux ELKS. Taken from sched.c/h of
 *	its big brother..
 *
 *	Copyright (C) 1991, 1992  Linus Torvalds
 *	Elks tweaks  (C) 1995 Alan Cox.
 *
 */
 
void add_wait_queue(p,wait)
register struct wait_queue **p;
register struct wait_queue *wait;
{
	flag_t flags;
	save_flags(flags);
	icli();
	if(!*p)
	{
		wait->next = wait;
		*p = wait;
	}
	else
	{
		wait->next = (*p)->next;
		(*p)->next = wait;
	}
	restore_flags(flags);
}

void remove_wait_queue(p,wait)
struct wait_queue **p;
register struct wait_queue *wait;
{
	flag_t flags;
	save_flags(flags);
	icli();
	if((*p==wait) && ((*p=wait->next)==wait))
		*p=NULL;
	else
	{
		register struct wait_queue *tmp=wait;
		while(tmp->next != wait)
			tmp = tmp->next;
		tmp->next = wait->next;
	}
	wait->next=NULL;
	restore_flags(flags);
}

int marker;

static void __sleep_on(p,state)
register struct wait_queue **p;
int state;
{
	flag_t flags;
	struct wait_queue wait;
	
	wait.next=NULL;
	wait.task = current;
	
	if(!p)
		return;
	if(current==&task[0]) {
		printk("task[0] trying to sleep ");
		panic("from %x", marker);
	}
	current->state = state;
	add_wait_queue(p, &wait);
	save_flags(flags);
	isti();
	schedule();
	remove_wait_queue(p, &wait);
	restore_flags(flags);
}

void sleep_on(p)
struct wait_queue **p;
{
	marker = peekw(get_ds(), get_bp() + 2);	
	__sleep_on(p, TASK_UNINTERRUPTIBLE);
}

void interruptible_sleep_on(p)
struct wait_queue **p;
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

void wake_up_process(p)
register struct task_struct * p;
{
	flag_t flags;

	save_flags(flags);
	icli();
	p->state = TASK_RUNNING;
	/*	if (p->pid == 5)
	  printk("adding task %d to runqueue\n", p->pid);*/
	if (!p->next_run)
		add_to_runqueue(p);
	restore_flags(flags);
}


/*
 * wake_up doesn't wake up stopped processes - they have to be awakened
 * with signals or similar.
 *
 * Note that this doesn't need cli-sti pairs: interrupts may not change
 * the wait-queue structures directly, but only call wake_up() to wake
 * a process. The process itself must remove the queue once it has woken.
 */

void _wake_up(q,it)
struct wait_queue **q;
int it;
{
	register struct wait_queue *tmp;
	register struct task_struct * p;

	if (!q || !(tmp = *q))
		return;
	do {
		if ((p = tmp->task) != NULL) {
			if ((it && (p->state == TASK_UNINTERRUPTIBLE))  ||
			    (p->state == TASK_INTERRUPTIBLE))
				wake_up_process(p);
		}
		if (!tmp->next) {
			printk("wait_queue is bad\n");
			printk("        q = %x\n",q);
			printk("       *q = %x\n",*q);
			printk("      tmp = %x\n",tmp);
			break;
		}
		tmp = tmp->next;
	} while (tmp != *q);
}

#ifdef NOTYET
void wake_up_interruptible(q)
struct wait_queue **q;
{
	register struct wait_queue *tmp;
	register struct task_struct * p;

	if (!q || !(tmp = *q))
		return;
	do {
		if ((p = tmp->task) != NULL) {
			if (p->state == TASK_INTERRUPTIBLE)
				wake_up_process(p);
		}
		if (!tmp->next) {
			printk("wait_queue is bad\n");
			printk("        q = %p\n",q);
			printk("       *q = %p\n",*q);
			printk("      tmp = %p\n",tmp);
			break;
		}
		tmp = tmp->next;
	} while (tmp != *q);
}

#endif
