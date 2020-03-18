#ifdef L_times
#include <sys/time.h>
#include <sys/times.h>

#if 0	/* system call not implemented*/

clock_t times(struct tms *tp)
{
   long rv;
   __times(tp, &rv);
   return rv;
}

#else

/* library version of 'times' since ELKS doesn't implement it*/
clock_t times(struct tms *tp)
{
	struct timeval tv;

	if (gettimeofday(&tv, (void *)0) < 0)
		return -1;

	if (tp) {
		/* scale down to one hour period to fit in long*/
		unsigned long usecs = (tv.tv_sec % 3600) * 1000000L + tv.tv_usec;

		/* return user and system same since ELKS doesn't implement*/
		tp->tms_utime = usecs;
		tp->tms_stime = usecs;
		tp->tms_cutime = usecs;
		tp->tms_cstime = usecs;
	}

	return tv.tv_sec;
}

#endif
#endif	/* L_times*/
