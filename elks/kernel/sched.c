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

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/timer.h>
#ifdef NEED_TQ
#include <linuxmt/tqueue.h>
#endif
#include <arch/irq.h>

#define init_task task[0]

int bh_mask_count[32];
unsigned long bh_mask = 0L;
void (*bh_base[32])();
unsigned long bh_active = 0L;
__ptask current, next;

static unsigned long lost_ticks = 0;
static unsigned long lost_ticks_system = 0;

extern struct wait_queue keywait;	/* For polled keyboard handling */
extern unsigned char can_tswitch;
extern int lastirq;

#ifdef NEED_TQ_TIMER
DECLARE_TASK_QUEUE(tq_timer);
#endif
#ifdef NEED_IMM_BH
DECLARE_TASK_QUEUE(tq_immediate);
#endif
#ifdef NEED_TQ_SCHEDULER
DECLARE_TASK_QUEUE(tq_scheduler);
#endif

/* int need_resched = 0; */
/* static int intr_count = 0; */

#if 0
/* This is a diagnostic dump of all currently running tasks... to be run after
 * fork() for now.  Not very intelligible yet */
void print_tasks()
{
	int i;
	__ptask pt;
	printk("# pid state ds\n");
	for (i = 0; i < MAX_TASKS; i++) {
		pt = &task[i];
		printk("%d %d %d %x %x\n", i, pt->pid, pt->state, pt->t_regs.cs, pt->t_regs.ds); 
	}
}
#endif

/*static inline*/ void add_to_runqueue(p)
register struct task_struct * p;
{
#if 0   /* sanity tests */ /* This is always pre-checked */
        if (p->next_run || p->prev_run) {
                printk("task already on run-queue\n");
                return;
        }
#endif
#if 0
        if (/*p->policy != SCHED_OTHER ||*/ p->counter > current->counter + 3)
                need_resched = 1;
#endif
	/*        nr_running++;*/
        (p->prev_run = init_task.prev_run)->next_run = p;
        p->next_run = &init_task;
        init_task.prev_run = p;

}

static /*inline*/ void del_from_runqueue(p)
struct task_struct * p;
{
        register struct task_struct *next = p->next_run;
        register struct task_struct *prev = p->prev_run;

#if 1   /* sanity tests */
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

	/*printk("process_timeout called!  data=%x, waking task %d\n", __data, p->pid);*/
        p->timeout = 0;
        wake_up_process(p);
}

static /*inline*/ int goodness(p, prev)
register struct task_struct * p;
struct task_struct * prev;
{
        int weight;

	    /*
	     * Realtime process, select the first one on the
	     * runqueue (taking priorities within processes
	     * into account).
	     */

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

/*
 *	Schedule a task. On entry current is the task, which will
 *	vanish quietly for a while and someone elses thread will return
 *	from here.
 */

void schedule()
{
	/* Including the two registers below saves lots of code, *
	 * but corrupts wait queue. */
        __uint c; 
	/* register */ __ptask prev; 	/* Subscript calculation is *very* expensive in bcc */
	__ptask next;
	/* register */ __ptask p;
	__ptask currentp = current;
	unsigned long flags, timeout = 0L;

	if (currentp->t_kstackm != KSTACK_MAGIC)
		panic("Process %d exceeded kernel stack limit! magic %x\n", 
			currentp->pid, currentp->t_kstackm);

	/* We have to let a task exit! */
	if (currentp->state == TASK_EXITING)
		return;

/* intr_count is not used in ELKS anywhere else */
/*	if (intr_count)
		goto scheduling_in_interrupt; */

	if (bh_active & bh_mask) {
/*		intr_count = 1; */
		do_bottom_half();
/*		intr_count = 0; */
	}

#ifdef NEED_TQ_SCHEDULER
	run_task_queue(&tq_scheduler);
#endif

#ifdef NEED_RSCHED
	need_resched = 0;
#endif

	prev = currentp;
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

	if (next != currentp) {
		struct timer_list timer;

		if (timeout) {
			init_timer(&timer);
			timer.expires = timeout;
			timer.data = (int) prev;
			timer.function = process_timeout;
			add_timer(&timer);
		}

		save_regs();
		if ((!can_tswitch) && (lastirq != -1)) 
			goto scheduling_in_interrupt;

		current = next;

		load_regs();	/* Returns to our caller */

		if (timeout) {
/*			printk("deleting timeout\n"); */
			del_timer(&timer);
		}
	}
	return;

scheduling_in_interrupt:
        printk("Aiee: scheduling in interrupt %d - %d %d\n",
                lastirq, currentp->pid, prev->pid);
}

#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)

#define SLOW_BUT_DEBUGGING_TIMERS 0

struct timer_vec {
        int index;
        struct timer_list *vec[TVN_SIZE];
};

struct timer_vec_root {
        int index;
        struct timer_list *vec[TVR_SIZE];
};

static struct timer_vec tv5 = { 0 };
static struct timer_vec tv4 = { 0 };
static struct timer_vec tv3 = { 0 };
static struct timer_vec tv2 = { 0 };
static struct timer_vec_root tv1 = { 0 };

static struct timer_vec * /*const*/ tvecs[] = {
	(struct timer_vec *)&tv1, &tv2, &tv3, &tv4, &tv5
};

#define NOOF_TVECS (sizeof(tvecs) / sizeof(tvecs[0]))

static unsigned long timer_jiffies = 0;

static /*inline*/ void insert_timer(timer, vec, idx)
register struct timer_list *timer;
register struct timer_list **vec;
int idx;
{
	if ((timer->next = vec[idx]))
		vec[idx]->prev = timer;
	vec[idx] = timer;
	timer->prev = (struct timer_list *)&vec[idx];
}

static /*inline*/ void internal_add_timer(timer)
register struct timer_list * timer;
{
	/*
	 * must be cli-ed when calling this
	 */
	unsigned long expires = timer->expires;
	unsigned long idx = expires - timer_jiffies;

	if (idx < TVR_SIZE) {
		int i = expires & TVR_MASK;
		insert_timer(timer, tv1.vec, i);
	} else if (idx < 1 << (TVR_BITS + TVN_BITS)) {
		int i = (expires >> TVR_BITS) & TVN_MASK;
		insert_timer(timer, tv2.vec, i);
	} else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS)) {
		int i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
		insert_timer(timer, tv3.vec, i);
	} else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS)) {
		int i = (expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
		insert_timer(timer, tv4.vec, i);
	} else if (expires < timer_jiffies) {
		/* can happen if you add a timer with expires == jiffies,
		 * or you set a timer to go off in the past
		 */
		insert_timer(timer, tv1.vec, tv1.index);
	} else if (idx < 0xffffffffUL) {
		int i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
		insert_timer(timer, tv5.vec, i);
	} else {
		/* Can only get here on architectures with 64-bit jiffies */
		timer->next = timer->prev = timer;
	}
}

void add_timer(timer)
struct timer_list * timer;
{
	unsigned long flags;
	save_flags(flags);
	icli();
#if SLOW_BUT_DEBUGGING_TIMERS
        if (timer->next || timer->prev) {
                printk("add_timer() called with non-zero list from %p\n",
		       __builtin_return_address(0));
		goto out;
        }
#endif
	internal_add_timer(timer);
#if SLOW_BUT_DEBUGGING_TIMERS
out:
#endif
	restore_flags(flags);
}

static /*inline*/ int detach_timer(timer)
struct timer_list * timer;
{
	int ret = 0;
	register struct timer_list *next;
	register struct timer_list *prev;
	next = timer->next;
	prev = timer->prev;
	if (next) {
		next->prev = prev;
	}
	if (prev) {
		ret = 1;
		prev->next = next;
	}
	return ret;
}


int del_timer(timer)
register struct timer_list * timer;
{
	int ret;
	unsigned long flags;
	save_flags(flags);
	icli();
	ret = detach_timer(timer);
	timer->next = timer->prev = 0;
	restore_flags(flags);
	return ret;
}

static /*inline*/ void cascade_timers(tv)
register struct timer_vec * tv;
{
        /* cascade all the timers from tv up one level */
        register struct timer_list *timer;
        timer = tv->vec[tv->index];
        /*
         * We are removing _all_ timers from the list, so we don't  have to
         * detach them individually, just clear the list afterwards.
         */
        while (timer) {
		struct timer_list *tmp = timer;
		timer = timer->next;
		internal_add_timer(tmp);
        }
        tv->vec[tv->index] = NULL;
        tv->index = (tv->index + 1) & TVN_MASK;
}

static /*inline*/ void run_timer_list()
{
	icli();
	while ((long)(jiffies - timer_jiffies) >= 0) {
		register struct timer_list *timer;
		if (!tv1.index) {
			int n = 1;
			do {
				cascade_timers(tvecs[n]);
			} while (tvecs[n]->index == 1 && ++n < NOOF_TVECS);
		}
		while ((timer = tv1.vec[tv1.index])) {
			void (*fn)() = timer->function;
			int data = timer->data;
			/*printk("boom!\n");*/
			detach_timer(timer);
			timer->next = timer->prev = NULL;
			isti();
			fn(data);
			icli();
		}
		++timer_jiffies; 
		tv1.index = (tv1.index + 1) & TVR_MASK;
	}
	isti();
}

static /*inline*/ void run_old_timers()
{
	register struct timer_struct *tp;
	unsigned long mask;

	for (mask = 1, tp = timer_table+0 ; mask ; tp++,mask += mask) {
		if (mask > timer_active)
			break;
		if (!(mask & timer_active))
			continue;
		if (tp->expires > jiffies)
			continue;
		timer_active &= ~mask;
		tp->fn();
		isti();
	}
}

#ifdef NEED_TQ_TIMER
void tqueue_bh()
{
	run_task_queue(&tq_timer);
}
#endif

#ifdef NEED_IMM_BH
void immediate_bh()
{
	run_task_queue(&tq_immediate);
}
#endif

unsigned long timer_active = 0;
struct timer_struct timer_table[32];

/* maybe someday I'll implement these profiling things -PL */
#if 0
static /*inline*/ void do_it_prof(p, ticks)
struct task_struct * p;
unsigned long ticks;
{
        unsigned long it_prof = p->it_prof_value;

        if (it_prof) {
	  if (it_prof <= ticks) {
                        it_prof = ticks + p->it_prof_incr;
                        send_sig(SIGPROF, p, 1);
	  }
                p->it_prof_value = it_prof - ticks;
        }
}
#endif

#if 0
static /*__inline__*/ void update_one_process(p, ticks, user, system)
struct taks_struct * p;
unsigned long ticks;
unsigned long user;
unsigned long system;
{
  /*do_process_times(p, user, system);*/
  /*do_it_virt(p, user);*/
  /*do_it_prof(p, ticks);*/
}
#endif

static void update_process_times(ticks, system)
unsigned long ticks;
unsigned long system;
{
        register struct task_struct * p = current;
        unsigned long user = ticks - system;
        if (p->pid) {
                p->counter -= ticks;
                if (p->counter < 0) {
                        p->counter = 0;
#ifdef NEED_RESCHED
                        need_resched = 1;
#endif
                }
#if 0
                if (p->priority < DEF_PRIORITY)
                        kstat.cpu_nice += user;
                else
                        kstat.cpu_user += user;
                kstat.cpu_system += system;
#endif
        }
        /*update_one_process(p, ticks, user, system);*/
}

static /*inline*/ void update_times()
{
        unsigned long ticks;

	icli();
	ticks = lost_ticks;
	lost_ticks = 0;
	isti();

        if (ticks) {
                unsigned long system;
		
		icli();
		system = lost_ticks_system;
		lost_ticks_system = 0;
		isti();

                /*calc_load(ticks);*/ /* don't care for now */
                /*update_wall_time(ticks);*/ /* this either */
                update_process_times(ticks, system);
        }
}

static void timer_bh()
{
  update_times();
  run_old_timers();
  run_timer_list();
}

void do_timer(regs)
struct pt_regs * regs;
{
        (*(unsigned long *)&jiffies)++;
        lost_ticks++;
        mark_bh(TIMER_BH);
#if 0
        if (!user_mode(regs)) {
                lost_ticks_system++;
                if (prof_buffer && current->pid) {
                        extern int _stext;
                        unsigned long ip = instruction_pointer(regs);
                        ip -= (unsigned long) &_stext;
                        ip >>= prof_shift;
                        if (ip < prof_len)
                                prof_buffer[ip]++;
                }
        }
#endif
#ifdef NEED_TQ_TIMER
        if (tq_timer)
                mark_bh(TQUEUE_BH);
#endif
}

void do_bottom_half()
{
        unsigned long active;
        unsigned long mask, left;
        void (**bh)();

        isti();
        bh = bh_base;
        active = bh_active & bh_mask;
        for (mask = 1, left = ~0 ; left & active ; bh++,mask += mask,left += left) {
	  if (mask & active) {
                        void (*fn)();
                        bh_active &= ~mask;
                        fn = *bh;
                        if (!fn)
                                goto bad_bh;
                        fn();
	  }
        }
        return;
bad_bh:
        printk ("irq.c:bad bottom half entry %08lx\n", mask);
}

void sched_init()
{
  init_bh(TIMER_BH, timer_bh);
#ifdef NEED_TQ_TIMER
  init_bh(TQUEUE_BH, tqueue_bh);
#endif
#ifdef NEED_IMM_BH
  init_bh(IMMEDIATE_BH, immediate_bh);
#endif
}


