#ifndef TIMER_H
#define TIMER_H

#include <sys/types.h>

/* timeq_t is the time type counted in 62.5ms (1/16 sec) quantums */
typedef	__u32 timeq_t;

#define TIME_LT(a,b)		((long)((a)-(b)) < 0)
#define TIME_LEQ(a,b)		((long)((a)-(b)) <= 0)
#define TIME_GT(a,b)		((long)((a)-(b)) > 0)
#define TIME_GEQ(a,b)		((long)((a)-(b)) >= 0)

extern timeq_t Now;

timeq_t timer_get_time(void);

#endif
