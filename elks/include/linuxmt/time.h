#ifndef __LINUXMT_TIME_H
#define __LINUXMT_TIME_H

#define DST_NONE        0	/* not on dst */
#define DST_USA         1	/* USA style dst */
#define DST_AUST        2	/* Australian style dst */
#define DST_WET         3	/* Western European dst */
#define DST_MET         4	/* Middle European dst */
#define DST_EET         5	/* Eastern European dst */
#define DST_CAN         6	/* Canada */
#define DST_GB          7	/* Great Britain and Eire */
#define DST_RUM         8	/* Rumania */
#define DST_TUR         9	/* Turkey */
#define DST_AUSTALT     10	/* Australian style with shift in 1986 */

#ifdef __KERNEL__

struct timeval {
    long tv_sec;		/* seconds */
    long tv_usec;		/* microseconds */
};

#define ITIMER_REAL     0

struct  itimerval {
    struct  timeval it_interval;/* timer interval */
    struct  timeval it_value;   /* current value */
};

struct timezone {
    int tz_minuteswest;		/* minutes west of Greenwich */
    int tz_dsttime;		/* type of dst correction */
};

#endif

#define NFDBITS                 __NFDBITS

#define FD_SETSIZE              __FD_SETSIZE
#define FD_SET(fd,fdsetp)       __FD_SET(fd,fdsetp)
#define FD_CLR(fd,fdsetp)       __FD_CLR(fd,fdsetp)
#define FD_ISSET(fd,fdsetp)     __FD_ISSET(fd,fdsetp)
#define FD_ZERO(fdsetp)         __FD_ZERO(fdsetp)

#endif
