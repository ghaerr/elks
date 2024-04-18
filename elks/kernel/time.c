/*
 * file:        time.c
 * description: kernel time functions
 * author:      Shane Kerr <kerr@wizard.net>
 * date:        23 October 1997
 *
 * note: The gettimeofday currently calculates the time using the system boot
 *       time and the number of ticks that have passed since that time.  This
 *       is pretty slow, but given that the time will probably be queried
 *       much less often than the timer ticks, probably more efficient than
 *       updating it properly in the timer IRQ handler.
 *
 * revision: 1998-01-01, kerr@wizard.net
 *     Added code for timezones. 
 *     Added settimeofday().
 *     Removed the code to set the clock on system boot - this is more
 *       properly done from a program that uses settimeofday() to set the time.
 *     Cleaned up gettimeofday() a bit.
 * 
 * note: The check for valid timezones in settimeofday() is a little cheesy,
 *       but I've decided to keep it the way it is in interests of saving 
 *       space.
 *
 * todo: adjtime() and/or adjtimex()
 */

#include <linuxmt/config.h>
#include <linuxmt/time.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
#include <linuxmt/string.h>

#include <arch/system.h>
#include <arch/segment.h>


/* this is the structure holding the base time (in UTC, of course) */
struct timeval xtime;
jiff_t xtime_jiffies;

/* timezone in effect */
static struct timezone xzone;

/* timezone offset (in hours) from CONFIG_TIME_TZ or /bootopts TZ= string */
int tz_offset;

void INITPROC tz_init(const char *tzstr)
{
    if (strlen(tzstr) > 3)
        tz_offset = atoi(tzstr+3);
}

time_t current_time(void)
{
    return (xtime.tv_sec + (jiffies - xtime_jiffies)/HZ);
}

/* set the time of day */
int sys_settimeofday(register struct timeval *tv, struct timezone *tz)
{
    struct timeval tmp_tv;
    struct timezone tmp_tz;

    /* only user running as root can set the time */
    if (current->euid != 0)
	return -EPERM;

    /* verify we have valid addresses to read from */
    if (tv != NULL) {
	if (verified_memcpy_fromfs(&tmp_tv, tv, sizeof(struct timeval)))
	    return -EFAULT;
	if (((unsigned long) tmp_tv.tv_usec) >= 1000000L)
	    return -EINVAL;
    }

    if (tz != NULL) {
	if (verified_memcpy_fromfs(&tmp_tz, tz, sizeof(struct timezone)))
	    return -EFAULT;
	if (((unsigned int)(tmp_tz.tz_dsttime - DST_NONE)) > (DST_AUSTALT - DST_NONE))
	    return -EINVAL;
	/* Setting the timezone is easy, just a straight copy */
	xzone = tmp_tz;
    }

    /* Setting time is a bit tricky, since we don't really keep the time in the xtime
     * structure.  So we need to figure out the offset from the current time and
     * set xtime based on that.
     * Too tricky: fix negative tv_usec bug, just store base time and
     * current jiffies for later use as offset - ghaerr.
     */
    if (tv != NULL) {
	xtime_jiffies = jiffies;
	xtime.tv_sec = tmp_tv.tv_sec;
	xtime.tv_usec = tmp_tv.tv_usec;
    }

    /* success */
    return 0;
}

/* return the time of day to the user */
int sys_gettimeofday(register struct timeval *tv, struct timezone *tz)
{
    struct timeval tmp_tv;
    jiff_t now;

    /* load the current time into the structures passed */
    if (tv != NULL) {
	now = jiffies;
	tmp_tv.tv_sec = xtime.tv_sec + (now - xtime_jiffies) / HZ;
	tmp_tv.tv_usec = xtime.tv_usec + ((now - xtime_jiffies) % HZ) * (1000000L / HZ);
	if (tmp_tv.tv_usec >= 10000000L)
	    tmp_tv.tv_usec -= 10000000L;
	if (verified_memcpy_tofs(tv, &tmp_tv, sizeof(struct timeval)))
	    return -EFAULT;
    }
    if (tz != NULL)
	if (verified_memcpy_tofs(tz, &xzone, sizeof(struct timezone)))
	    return -EFAULT;

    /* success */
    return 0;
}
