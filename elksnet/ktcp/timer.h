#ifndef __TIMER_H__
#define __TIMER_H__

#include <sys/types.h>

/*
 * time_t is the time type counted in
 * 62ms quantums
 */
typedef	__u32 timeq_t;

#define TIME_LT(a,b)		((long)((a)-(b)) < 0)
#define TIME_LEQ(a,b)		((long)((a)-(b)) <= 0)
#define TIME_GT(a,b)		((long)((a)-(b)) > 0)
#define TIME_GEQ(a,b)		((long)((a)-(b)) >= 0)

timeq_t	Now;

#endif

