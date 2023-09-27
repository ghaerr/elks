#include <linuxmt/sched.h>
#include <linuxmt/timer.h>
#include <linuxmt/kernel.h>
#include <linuxmt/signal.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>
#include <arch/segment.h>
#include <arch/io.h>
/*
 * Alarm system call
 *
 * Feb 2022 Greg Haerr
 */

static struct timer_list alarms[NR_ALARMS];

static void alarm_callback(int data)
{
	struct task_struct *p = (struct task_struct *)data;

	debug("kernel ALARM pid %P for %d\n", p->pid);
	send_sig(SIGALRM, p, 1);
}

static struct timer_list *find_alarm(struct task_struct *t)
{
	struct timer_list *ap;

	for (ap = alarms; ap < &alarms[NR_ALARMS]; ap++ ) {
		if (ap->tl_data == (int)t)
			return ap;
	}
	return NULL;
}

unsigned int sys_alarm(unsigned int secs)
{
	struct timer_list *ap;

	debug("sys_alarm %d\n", secs);
	ap = find_alarm(current);
	if (secs == 0) {
		if (ap) {
			del_timer(ap);
			ap->tl_data = 0;
			return 0;
		}
	} else {
		if (!ap && !(ap = find_alarm(NULL))) {
			printk("No more alarms\n");
			return 0;
		}
		del_timer(ap);
		ap->tl_expires = jiffies + ((unsigned long)secs * HZ);
		ap->tl_function = alarm_callback;
		ap->tl_data = (int)current;	/* must delete timer on process exit*/
		add_timer(ap);
	}
	return 0;
}
