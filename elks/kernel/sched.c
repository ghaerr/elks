/*
 *  kernel/sched.c
 *  (C) 1995 Chad Page 
 *  
 *  This is the main scheduler - hopefully simpler than Linux's at present.
 * 
 *
 */

/* Commnent in below to use the old scheduler which uses counters */

#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/timer.h>

#include <arch/irq.h>

#define init_task task[0]

__task task[MAX_TASKS];
unsigned char nr_running;

__ptask current, next, previous;

extern unsigned char can_tswitch;
extern int lastirq;

extern int do_signal(void);

static void run_timer_list();

void add_to_runqueue(register struct task_struct *p)
{
    nr_running++;
    (p->prev_run = init_task.prev_run)->next_run = p;
    p->next_run = &init_task;
    init_task.prev_run = p;
}

void del_from_runqueue(register struct task_struct *p)
{
#if 0       /* sanity tests */
    if (!p->next_run || !p->prev_run) {
    printk("task %d not on run-queue (state=%d)\n", p->pid, p->state);
    return;
    }
#endif
    if (p == &init_task) {
        printk("idle task may not sleep\n");
        return;
    }
    nr_running--;
    p->next_run->prev_run = p->prev_run;
    p->prev_run->next_run = p->next_run;
    p->next_run = p->prev_run = NULL;
#ifdef CONFIG_SWAP
    p->last_running = jiffies;
#endif

}


static void process_timeout(int __data)
{
    register struct task_struct *p = (struct task_struct *) __data;

#if 0
    printk("process_timeout called!  data=%x, waking task %d\n", __data,
       p->pid);
#endif
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
    __ptask currentp = current;
    jiff_t timeout = 0L;

    prev = current;
    next = prev->next_run;

    if (prev->t_kstackm != KSTACK_MAGIC)
        panic("Process %d exceeded kernel stack limit! magic %x\n", 
            currentp->pid, currentp->t_kstackm);

    /* We have to let a task exit! */
    if (prev->state == TASK_EXITING)
        return;

    run_timer_list();
    
    clr_irq();
    switch (prev->state) {
    case TASK_INTERRUPTIBLE:
        if (prev->signal /* & ~prev->blocked */ )
            goto makerunnable;
        
        timeout = prev->timeout;
    
        if (timeout && (timeout <= jiffies)) {
           prev->timeout = timeout = 0;
makerunnable:
            prev->state = TASK_RUNNING;
            break;
        }
    
    default:
        del_from_runqueue(prev);
        /*break; */
    case TASK_RUNNING:
        break;
    }
    set_irq();
    
    while(next == &init_task && nr_running > 0){
        next = next->next_run;
    }

    if (next != currentp) {
        struct timer_list timer;

        if (timeout) {
            init_timer(&timer);
            timer.tl_expires = timeout; 
            timer.tl_data = (int) prev;
            timer.tl_function = process_timeout;
            add_timer(&timer);
        }

        if ((!can_tswitch) && (lastirq != -1))
            goto scheduling_in_interrupt;

#ifdef CONFIG_SWAP
        if(do_swapper_run(next) == -1){
            printk("Can't become runnable %d\n", next->pid);
            panic("");
        }
#endif

        previous = current; 
        current = next;

        tswitch();  /* Won't return for a new task */

        if(current->signal){
            do_signal();
            current->signal = 0;
        }
                
        if (timeout) {
            del_timer(&timer);
        }
    }

    return;

scheduling_in_interrupt:

    /* Taking a timer IRQ during another IRQ or while in kernel space is
     * quite legal. We just dont switch then */
    if (lastirq > 0)
        printk("Aiee: scheduling in interrupt %d - %d %d\n",
           lastirq, currentp->pid, prev->pid);
}

struct timer_list tl_list = { NULL, NULL, NULL, NULL, NULL };

static int detach_timer(struct timer_list *timer)
{
    int ret = 0;
    register struct timer_list *next;
    register struct timer_list *prev;
    next = timer->tl_next;
    prev = timer->tl_prev;
    if (next) {
        next->tl_prev = prev;
    }
    if (prev) {
        ret = 1;
        prev->tl_next = next;
    }
    return ret;
}

int del_timer(register struct timer_list *timer)
{
    int ret;
    flag_t flags;
    save_flags(flags);
    clr_irq();
    ret = detach_timer(timer);
    timer->tl_next = timer->tl_prev = 0;
    restore_flags(flags);
    return ret;
}

void init_timer(register struct timer_list *timer)
{
    timer->tl_next = timer->tl_prev = NULL;
}

void add_timer(register struct timer_list *timer)
{
    flag_t flags;
    register struct timer_list *next = tl_list.tl_next;
    struct timer_list *prev = &tl_list;

    save_flags(flags);
    clr_irq();

    while (next) {
        if (next->tl_expires > timer->tl_expires) {
            timer->tl_prev = next->tl_prev;
            timer->tl_next = next;
            timer->tl_prev->tl_next = timer;
            next->tl_prev = timer;
            restore_flags(flags);
            return;
        }
        prev = next;
        next = next->tl_next;
    }
    (timer->tl_prev = prev)->tl_next = timer;

#if 0
    timer->tl_next = NULL;
#endif

    restore_flags(flags);
}

static void run_timer_list(void)
{
    register struct timer_list *timer = tl_list.tl_next;

    clr_irq();
    while (timer && timer->tl_expires < jiffies) {
        void (*fn) () = timer->tl_function;
        int data = timer->tl_data;
        detach_timer(timer);
        timer->tl_next = timer->tl_prev = NULL;
        set_irq();
        fn(data);
        clr_irq();
        timer = timer->tl_next;
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
    (*(jiff_t *) & jiffies)++;
}

void sched_init(void)
{
    /* Do nothing */ ;
}
