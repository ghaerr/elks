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

#include <arch/types.h>
#include <linuxmt/time.h>
#include <linuxmt/timex.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>

#include <linuxmt/config.h>
#include <arch/system.h>
#include <arch/segment.h>

#include <linuxmt/sched.h>

/* this is the structure holding the base time (in UTC, of course) */
struct timeval xtime;

/* ticks updated by the timer interrupt, to be added to the base time */
extern jiff_t jiffies;

/* timezone in effect */
static struct timezone xzone;

/* set the time of day */
int sys_settimeofday(struct timeval *tv, register struct timezone *tz)
{
    int error;
    struct timeval tmp_tv;
    struct timezone tmp_tz;
    jiff_t now;

    /* only user running as root can set the time */
    if (current->euid != 0)
	return -EPERM;

    /* verify we have valid addresses to read from */
    if (tv != NULL) {
	if (error =
	    verified_memcpy_fromfs(&tmp_tv, tv, sizeof(struct timeval))) {
	    return error;
	}
	if ((tmp_tv.tv_usec < 0) || (tmp_tv.tv_usec >= 1000000L)) {
	    return -EINVAL;
	}
    }
    if (tz != NULL) {
	if (error =
	    verified_memcpy_fromfs(&tmp_tz, tz,
				   sizeof(struct timezone))) return error;
	if ((tmp_tz.tz_dsttime < DST_NONE)
	    || (tmp_tz.tz_dsttime > DST_AUSTALT)) {
	    return -EINVAL;
	}
    }

    /* Setting time is a bit tricky, since we don't really keep the time in the xtime
     * structure.  So we need to figure out the offset from the current time and
     * set xtime based on that.
     */
    if (tv != NULL) {
	now = jiffies;
	xtime.tv_sec = tmp_tv.tv_sec - (now / HZ);
	xtime.tv_usec = tmp_tv.tv_usec - ((now % HZ) * (1000000l / HZ));
    }

    /* Setting the timezone is easier, just a straight copy */
    if (tz != NULL) {
	xzone = tmp_tz;
    }

    /* success */
    return 0;
}

/* return the time of day to the user */
int sys_gettimeofday(register struct timeval *tv, struct timezone *tz)
{
    int error;
    struct timeval tmp_tv;
    jiff_t now;

    /* load the current time into the structures passed */
    if (tv != NULL) {
	now = jiffies;
	tmp_tv.tv_sec = xtime.tv_sec + (now / HZ);
	tmp_tv.tv_usec = xtime.tv_usec + ((now % HZ) * (1000000l / HZ));
	if (error = verified_memcpy_tofs(tv, &tmp_tv, sizeof(struct timeval)))
	    return error;
    }
    if (tz != NULL) {
	if (error = verified_memcpy_tofs(tz, &xzone, sizeof(struct timezone)))
	    return error;
    }

    /* success */
    return 0;
}
