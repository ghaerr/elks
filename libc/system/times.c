#ifdef L_times
#include <sys/time.h>
#include <sys/times.h>

/*
 * Library version of 'times' since ELKS doesn't implement the system call.
 * Note: this routine returns the system clock time in microseconds,
 * not the accumulated CPU time(s) nor in CLK_TCKs.
 * Thus, it should not be used and has been removed from compilation.
 */
clock_t times(struct tms *tp)
{
        struct timeval tv;

        if (gettimeofday(&tv, (void *)0) < 0)
                return -1;

        if (tp) {
                clock_t usecs = tv.tv_sec * 1000000LL + tv.tv_usec;

                /* return user and system same since ELKS doesn't implement*/
                tp->tms_utime = usecs;
                tp->tms_stime = usecs;
                tp->tms_cutime = usecs;
                tp->tms_cstime = usecs;
        }

        return tv.tv_sec;
}
#endif	/* L_times*/
