/*
 * 	kernel/sched.c
 * 	(C) 1995 Chad Page 
 *	
 *	This is the main scheduler - hopefully simpler than Linux's at present.
 * 
 * 	We need to implement run-queues RSN, but first we just have to make
 * 	it *work*.  Of course, writing it would be a good start :)
 *
 */

/* Commnent in below to use the old scheduler which uses counters */
/* #define OLD_SCHED */


#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/timer.h>
#include <arch/irq.h>

#define init_task task[0]

__task task[MAX_TASKS];

void (*bh_base[16])();
unsigned bh_active = 0;
__ptask current, next, previous;

#ifdef OLD_SCHED
static jiff_t lost_ticks = 0L;
static jiff_t lost_ticks_system = 0L;
#endif /* OLD_SCHED */

extern unsigned char can_tswitch;
extern int lastirq;

static void run_timer_list();
static void run_old_timers();
/* int need_resched = 0; */
/* static int intr_count = 0; */

void add_to_runqueue(p)
register struct task_struct * p;
{
#ifdef OLD_SCHED
        if (p->counter > current->counter + 3)
                need_resched = 1;
#endif
	/*        nr_running++;*/
        (p->prev_run = init_task.prev_run)->next_run = p;
        p->next_run = &init_task;
        init_task.prev_run = p;

}

void del_from_runqueue(p)
struct task_struct * p;
{
        register struct task_struct *next = p->next_run;
        register struct task_struct *prev = p->prev_run;

#if 0   /* sanity tests */
        if (!next || !prev) {
                printk("task %d not on run-queue (state=%d)\n", p->pid,
		       p->state);
                return;
        }
#endif
        if (p == &init_task) {
		printk("idle task may not sleep\n");
                return;
        }
	/*        nr_running--;*/
        next->prev_run = prev;
        prev->next_run = next;
        p->next_run = NULL;
        p->prev_run = NULL;
}


static void process_timeout(__data)
int __data;
{
        register struct task_struct * p = (struct task_struct *) __data;
/*	printk("process_timeout called!  data=%x, waking task %d\n", __data, p->pid); */
        p->timeout = 0UL;
        wake_up_process(p);
}

#ifdef OLD_SCHED
static /*inline*/ int goodness(p, prev)
register struct task_struct * p;
struct task_struct * prev;
{
        int weight;

        /*
         * Give the process a first-approximation goodness value
         * according to the number of clock-ticks it has left.
         *
         * Don't do any other calculations if the time slice is
         * over..
         */
        weight = p->counter;
        if (weight) {
                /* .. and a slight advantage to the current process */
                if (p == prev)
                        weight += 1;
        }

        return weight+1000;
}
#endif

/*
 *	Schedule a task. On entry current is the task, which will
 *	vanish quietly for a while and someone elses thread will return
 *	from here.
 */

void schedule()
{
	/* Including the two registers below saves lots of code, *
	 * but corrupts wait queue. */
#ifdef OLD_SCHED
        __uint c; 
	__ptask p;
#endif
	/* register */ __ptask prev; 	/* Subscript calculation is *very* expensive in bcc */
	__ptask next;
	__ptask currentp = current;
	jiff_t timeout = 0L;

	if (currentp->t_kstackm != KSTACK_MAGIC)
		panic("Process %d exceeded kernel stack limit! magic %x\n", 
			currentp->pid, currentp->t_kstackm);

	/* We have to let a task exit! */
	if (currentp->state == TASK_EXITING)
		return;

	run_timer_list();

#ifdef OLD_SCHED
	need_resched = 0;
#endif

	prev = currentp;
	next = prev->next_run;
	icli();

	switch (prev->state) {
		case TASK_INTERRUPTIBLE:
/*			printk("going to int task\n");*/
			if (prev->signal/* & ~prev->blocked*/)
				goto makerunnable;
			timeout = prev->timeout;
/*			printk("jiffies = %d/%d, timeout = %d/%d\n", jiffies, timeout);*/
			if (timeout && (timeout <= jiffies)) {
				prev->timeout = 0;
				timeout = 0;
		makerunnable:
/*				printk("sched: int -> run\n");*/
				prev->state = TASK_RUNNING;
				break;
			}
		default: 
			del_from_runqueue(prev);
			/*break;*/
		case TASK_RUNNING: break;
	}

#ifdef OLD_SCHED
	p = init_task.next_run;
	/*if (init_task.next_run->pid != 0)
	  printk("init_task.next_run->pid=%d\n", init_task.next_run->pid);*/
	isti();

	c = 0;
	next = &init_task;
	while (p != &init_task) {
		int weight = goodness(p, prev);
		/*printk("checking out task %d (state=%d, goodness=%d), next=%d\n",
		       p->pid, p->state, weight, p->next_run->pid); */
		if (weight > c)
			c = weight, next = p;
		p = p->next_run;
	}

	/*	if (next->pid != 0)
	printk("chose task %d (state=%d) to run\n", next->pid, next->state);*/

        /* if all runnable processes have "counter == 0", re-calculate counters
         */
	if (c==1000) {
		for_each_task(p)
			p->counter = (p->counter >> 1) + p->t_priority;
	}
#else
	isti();
	while ((next != &init_task) && (next == prev)) {
		next = next->next_run;
	}

#endif

	if (next != currentp) {
		struct timer_list timer;
		int foo = 10;

/*		printk("Switching to %x\n", next); */
		if (timeout) {
			init_timer(&timer);
			timer.tl_expires = timeout;
			timer.tl_data = (int) prev;
			timer.tl_function = process_timeout;
			add_timer(&timer);
		}
	/* The code below has been changed as the old task switching code did not return
	   here, which meant that defunct timers were not getting deleted which caused
	   a kernel panic when the timer ran out.
	   The new switcher consists of one function which switches from the task pointed
	   to by previous, to that pointed to by current, instead of the old save_regs() 
	   current = next load_regs() arrangement. Th is new arrangement allows us to return
	   to the same point, except with a new task.
	   Al <ajr@ecs.soton.ac.uk> 4th May 1999 */

/*		save_regs();/* */
		if ((!can_tswitch) && (lastirq != -1)) 
			goto scheduling_in_interrupt;

		previous = current; /* */
		current = next;

		tswitch();	/* Won't return for a new task */

		if (timeout) {
			del_timer(&timer);
		}
	}
	return;

scheduling_in_interrupt:
	/* Taking a timer IRQ during another IRQ or while in kernel space is
	   quite legal. We just dont switch then */
	if(lastirq > 0)
	        printk("Aiee: scheduling in interrupt %d - %d %d\n",
        	        lastirq, currentp->pid, prev->pid);
}

struct timer_list tl_list = {NULL,};

static int detach_timer(timer)
struct timer_list * timer;
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


int del_timer(timer)
register struct timer_list * timer;
{
	int ret;
	flag_t flags;
	save_flags(flags);
	icli();
	ret = detach_timer(timer);
	timer->tl_next = timer->tl_prev = 0;
	restore_flags(flags);
	return ret;
}

static void init_timer(timer)
struct timer_list * timer;
{
	timer->tl_next = NULL;
	timer->tl_prev = NULL;
}


static void add_timer(timer)
register struct timer_list * timer;
{
	flag_t flags;
	struct timer_list * next = tl_list.tl_next;
	struct timer_list * prev = &tl_list;

	save_flags(flags);
	icli();

	while (next) {
		if (next->tl_expires > timer->tl_expires) {
			timer->tl_prev = next->tl_prev;
			timer->tl_next = next;
			timer->tl_prev->tl_next = timer;
			next->tl_prev = timer;
			return;
		}
		prev = next;
		next = next->tl_next;
	}
	timer->tl_prev = prev;
/*	timer->tl_next = NULL; */
	prev->tl_next = timer;
	restore_flags(flags);
}

static void run_timer_list()
{
	struct timer_list * timer = tl_list.tl_next;

	icli();

	while (timer && timer->tl_expires < jiffies) {
		void (*fn)() = timer->tl_function;
		int data = timer->tl_data;
		detach_timer(timer);
		timer->tl_next = timer->tl_prev = NULL;
		isti();
		fn(data);
		icli();
		timer = timer->tl_next;
	}

	isti();
}


/* maybe someday I'll implement these profiling things -PL */
#if 0
static void do_it_prof(p, ticks)
struct task_struct * p;
jiff_t ticks;
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

static void update_one_process(p, ticks, user, system)
struct taks_struct * p;
jiff_t ticks;
jiff_t user;
jiff_t system;
{
	do_process_times(p, user, system);
	do_it_virt(p, user);
	do_it_prof(p, ticks);
}
#endif

#ifdef OLD_SCHED
static void update_times()
{
        jiff_t ticks;

	icli();
	ticks = lost_ticks;
	lost_ticks = 0;
	isti();

        if (ticks) {
		register struct task_struct * p = current;
		
                /*calc_load(ticks);*/ /* don't care for now */
                /*update_wall_time(ticks);*/ /* this either */
		if (p->pid) {
			p->counter -= ticks;
			if (p->counter < 0) {
				p->counter = 0;
				need_resched = 1;
			}
		}
        }
}
#endif /* OLD_SCHED */

void do_timer(regs)
struct pt_regs * regs;
{
        (*(jiff_t *)&jiffies)++;
#ifdef OLD_SCHED
        lost_ticks++;
#endif /* OLD_SCHED */
}

void sched_init()
{
}


