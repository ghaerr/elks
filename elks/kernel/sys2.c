#include <linuxmt/sched.h>
#include <linuxmt/timer.h>
#include <linuxmt/kernel.h>
#include <linuxmt/signal.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
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

	debug("(%P)ALARM for pid %d\n", p->pid);
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

static int setalarm(unsigned long jiffs)
{
	struct timer_list *ap;

	debug("(%P)sys_alarm %d\n", secs);
	ap = find_alarm(current);
	if (jiffs == 0) {
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
		ap->tl_expires = jiffies + jiffs;
		ap->tl_function = alarm_callback;
		ap->tl_data = (int)current;	/* must delete timer on process exit*/
		add_timer(ap);
	}
	return 0;
}

unsigned int sys_alarm(unsigned int secs)
{
    return setalarm((unsigned long)secs * HZ);
}

/* NOTE: itimer.it_interval not yet implemented */
int sys_setitimer(int which, struct itimerval *value, struct itimerval *ovalue)
{
    unsigned long sec, usec, jiffs;
    struct itimerval itv;

    if (which != ITIMER_REAL)
        return -EINVAL;
    if (value) {
        if (verified_memcpy_fromfs(&itv, value, sizeof(struct itimerval)))
            return -EFAULT;
    } else {
        itv.it_value.tv_sec = 0;
        itv.it_value.tv_usec = 0;
    }

    sec = itv.it_value.tv_sec;
    usec = itv.it_value.tv_usec;
    if (sec > (ULONG_MAX / HZ))
        jiffs = ULONG_MAX;
    else {
        usec += 1000000 / HZ - 1;
        usec /= 1000000 / HZ;
        jiffs = HZ * sec + usec;
    }
    return setalarm(jiffs);
}
