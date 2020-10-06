/*
 *  kernel/sched.c
 *  (C) 1995 Chad Page
 *
 *  This is the main scheduler - hopefully simpler than Linux's at present.
 */

#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/init.h>
#include <linuxmt/timer.h>
#include <linuxmt/string.h>
#include <linuxmt/debug.h>

#include <arch/irq.h>

#define idle_task task[0]

__task task[MAX_TASKS];
__ptask current = task;
__ptask previous;

//static unsigned char nr_running;
extern int intr_count;

static void run_timer_list();

void add_to_runqueue(register struct task_struct *p)
{
    //nr_running++;
    (p->prev_run = idle_task.prev_run)->next_run = p;
    p->next_run = &idle_task;
    idle_task.prev_run = p;
}

void del_from_runqueue(register struct task_struct *p)
{
#if 0       /* sanity tests */
    if (!p->next_run || !p->prev_run) {
	printk("task %d not on run-queue (state=%d)\n", p->pid, p->state);
	return;
    }
    if (p == &idle_task) {
        printk("idle task may not sleep\n");
        return;
    }
#endif
    //nr_running--;
    (p->next_run->prev_run = p->prev_run)->next_run = p->next_run;
    p->next_run = p->prev_run = NULL;

}


static void process_timeout(int __data)
{
    register struct task_struct *p = (struct task_struct *) __data;

    debug_sched("sched: timeout %d\n", p->pid);
    p->timeout = 0UL;
    wake_up_process(p);
}

/*
 *  Schedule a task. On entry current is the task, which will
 *  vanish quietly for a while and someone elses thread will return
 *  from here.
 */

void schedule(void)
{
    register __ptask prev;
    register __ptask next;
    struct timer_list timer;
    jiff_t timeout = 0UL;

    prev = current;

    if (prev->t_kstackm != KSTACK_MAGIC)
        panic("Process %d exceeded kernel stack limit! magic %x\n",
            prev->pid, prev->t_kstackm);

    if (intr_count > 0) {
    /* Taking a timer IRQ during another IRQ or while in kernel space is
     * quite legal. We just dont switch then */
	printk("Aiee: scheduling in interrupt %d - %d\n", intr_count, prev->pid);
	return;
    }

    /* We have to let a task exit! */
    if (prev->state == TASK_EXITING)
	return;

    clr_irq();
    if (prev->state == TASK_INTERRUPTIBLE) {
        if (prev->signal || (prev->timeout && (prev->timeout <= jiffies))) {
            prev->timeout = 0UL;
            prev->state = TASK_RUNNING;
        }
        else {
	    timeout = prev->timeout;
	}
    }

    /* Choose a task to run next */
    next = prev->next_run;
    if (prev->state != TASK_RUNNING)
	del_from_runqueue(prev);
    if (next == &idle_task)
        next = next->next_run;
    set_irq();

    if (next != prev) {

        if (timeout) {
            init_timer(&timer);
            timer.tl_expires = timeout;
            timer.tl_data = (int) prev;
            timer.tl_function = process_timeout;
            add_timer(&timer);
        }

        previous = prev;
        current = next;
	debug_sched("sched: %d\n", current->pid);
        tswitch();  /* Won't return for a new task */

        if (timeout) {
            del_timer(&timer);
        }
    } else if (current->pid)
	debug_sched("resched: %d prevstate %d\n", current->pid, prev->state);
}

struct timer_list tl_list = { NULL, NULL, 0L, 0, NULL };

void init_timer(register struct timer_list *timer)
{
    timer->tl_next = timer->tl_prev = NULL;
}

static int detach_timer(register struct timer_list *timer)
{
    register struct timer_list *tmr;
    int retval = 0;

    if ((tmr = timer->tl_next)) {
        tmr->tl_prev = timer->tl_prev;
    }
    if ((tmr = timer->tl_prev)) {
        tmr->tl_next = timer->tl_next;
	retval = 1;
    }
    init_timer(timer);
    return retval;
}

int del_timer(struct timer_list *timer)
{
    int ret;
    flag_t flags;

    save_flags(flags);
    clr_irq();
    ret = detach_timer(timer);
    restore_flags(flags);
    return ret;
}

void add_timer(register struct timer_list *timer)
{
    flag_t flags;
    register struct timer_list *next = &tl_list;
    struct timer_list *prev;

    save_flags(flags);
    clr_irq();

    do {
        prev = next;
    } while ((next = next->tl_next) && (next->tl_expires < timer->tl_expires));

    (timer->tl_prev = prev)->tl_next = timer;
    if ((timer->tl_next = next))
        next->tl_prev = timer;

    restore_flags(flags);
}

static void run_timer_list(void)
{
    register struct timer_list *timer;

    clr_irq();
    while ((timer = tl_list.tl_next) && timer->tl_expires <= jiffies) {
        detach_timer(timer);
        set_irq();
        timer->tl_function(timer->tl_data);
        clr_irq();
    }
    set_irq();
}

/* maybe someday I'll implement these profiling things -PL */

#if 0

static void do_it_prof(struct task_struct *p, jiff_t ticks)
{
    jiff_t it_prof = p->it_prof_value;

    if (it_prof) {
    if (it_prof <= ticks) {
        it_prof = ticks + p->it_prof_incr;
        send_sig(SIGPROF, p, 1);
    }
    p->it_prof_value = it_prof - ticks;
    }
}

static void update_one_process(struct taks_struct *p,
                   jiff_t ticks, jiff_t user, jiff_t system)
{
    do_process_times(p, user, system);
    do_it_virt(p, user);
    do_it_prof(p, ticks);
}

#endif

void do_timer(struct pt_regs *regs)
{
    jiffies++;

#ifdef NEED_RESCHED		/* need_resched is not checked anywhere */
    if (!((int) jiffies & 7))
	need_resched = 1;	/* how primitive can you get? */
#endif

    run_timer_list();

}

void INITPROC sched_init(void)
{
    register struct task_struct *t = &task[MAX_TASKS];

/*
 *	Mark tasks 0-(MAX_TASKS-1) as not in use.
 */
    do {
	(--t)->state = TASK_UNUSED;
    } while (t > task);

/*
 *	Now create task 0 to be ourself.
 */
    kfork_proc(NULL);

    t->state = TASK_RUNNING;
    t->next_run = t->prev_run = t;
}
