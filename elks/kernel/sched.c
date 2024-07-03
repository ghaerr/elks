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
#include <linuxmt/trace.h>
#include <linuxmt/debug.h>

#include <arch/irq.h>

#define idle_task task[0]

struct task_struct *task;           /* dynamically allocated task array */
struct task_struct *current;
struct task_struct *previous;
int max_tasks = MAX_TASKS;

void add_to_runqueue(register struct task_struct *p)
{
    (p->prev_run = idle_task.prev_run)->next_run = p;
    p->next_run = &idle_task;
    idle_task.prev_run = p;
}

static void del_from_runqueue(register struct task_struct *p)
{
#ifdef CHECK_SCHED
    if (!p->next_run || !p->prev_run)
        panic("SCHED(%d): task not on run-queue, state %d", p->pid, p->state);
    if (p == &idle_task)
        panic("SCHED: trying to sleep idle task");
#endif
    (p->next_run->prev_run = p->prev_run)->next_run = p->next_run;
    p->next_run = p->prev_run = NULL;

}

static void process_timeout(int __data)
{
    struct task_struct *p = (struct task_struct *) __data;

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
    struct task_struct *prev;
    struct task_struct *next;
    jiff_t timeout = 0UL;
    struct timer_list timer;

    prev = current;

#ifdef CHECK_SCHED
    if (intr_count) {
        /* Taking a timer IRQ during another IRQ or while in kernel space is
         * quite legal. We just dont switch then */
         panic("SCHED: schedule() called from interrupt, intr_count %d", intr_count);
    }
#endif

    /* Disallow rescheduling during startup when idle task is the only task */
    if ((int)last_pid <= 0)
        return;

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
            timer.tl_expires = timeout;
            timer.tl_data = (int) prev;
            timer.tl_function = process_timeout;
            add_timer(&timer);
        }

        previous = prev;
        current = next;
        debug_sched("sched: %P\n");
        tswitch();  /* Won't return for a new task */

        if (timeout) {
            del_timer(&timer);
        }
    } else if (current->pid)
        debug_sched("resched: %P prevstate %d\n", prev->state);
}

static struct timer_list *next_timer;

void add_timer(struct timer_list * timer)
{
    struct timer_list **p;
    flag_t flags;

    timer->tl_next = NULL;
    p = &next_timer;
    save_flags(flags);
    clr_irq();
    while (*p) {
        if ((*p)->tl_expires > timer->tl_expires) {
            timer->tl_next = *p;
            break;
        }
        p = &(*p)->tl_next;
    }
    *p = timer;
    restore_flags(flags);
}

int del_timer(struct timer_list * timer)
{
    struct timer_list **p;
    flag_t flags;

    p = &next_timer;
    save_flags(flags);
    clr_irq();
    while (*p) {
        if (*p == timer) {
            *p = timer->tl_next;
            restore_flags(flags);
            return 1;
        }
        p = &(*p)->tl_next;
    }
    restore_flags(flags);
    return 0;
}

static void run_timer_list(void)
{
    struct timer_list *timer;

    clr_irq();
    while ((timer = next_timer) && timer->tl_expires <= jiffies) {
        del_timer(timer);
        set_irq();
        timer->tl_function(timer->tl_data);
        clr_irq();
    }
    set_irq();
}

void do_timer(void)
{
    jiffies++;

    /***if (!((int) jiffies & 7))
        need_resched = 1;***/       /* how primitive can you get? */

    run_timer_list();

}

void INITPROC sched_init(void)
{
    struct task_struct *t = &task[max_tasks];

/*
 *  Mark tasks 0-(max_tasks-1) as not in use.
 */
    do {
        (--t)->state = TASK_UNUSED;
    } while (t > task);

    current = task;
    next_task_slot = task;
    task_slots_unused = max_tasks;
/*
 *  Now create task 0 to be ourself.
 */
    kfork_proc(NULL);

    t->state = TASK_RUNNING;
    t->next_run = t->prev_run = t;
}
