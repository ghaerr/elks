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

static struct timer_list alarm;

static void alarm_callback(int data)
{
	struct task_struct *p = (struct task_struct *)data;

	debug("kernel ALARM pid %d for %d\n", current->pid, p->pid);
	send_sig(SIGALRM, p, 1);
}

unsigned int sys_alarm(unsigned int secs)
{
	debug("sys_alarm %d\n", secs);
	if (secs == 0) {
		if (alarm.tl_data)
			del_timer(&alarm);
		alarm.tl_data = 0;
	} else {
		init_timer(&alarm);
		alarm.tl_expires = jiffies + (secs * HZ);
		alarm.tl_function = alarm_callback;
		alarm.tl_data = (int)current;	/* must delete timer on process exit*/
		add_timer(&alarm);
	}
	return 0;
}
