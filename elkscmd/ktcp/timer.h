#ifndef TIMER_H
#define TIMER_H

#include <sys/types.h>

/* timeq_t used to be the time type counted in 62.5ms (1/16 sec) quantums */
/* Now (2025) it's simply jiffies. */
typedef	__u32 timeq_t;

#define TIME_LT(a,b)		((long)((a)-(b)) < 0)
#define TIME_LEQ(a,b)		((long)((a)-(b)) <= 0)
#define TIME_GT(a,b)		((long)((a)-(b)) > 0)
#define TIME_GEQ(a,b)		((long)((a)-(b)) >= 0)

extern timeq_t Now;
extern unsigned long __far *jp;

int timer_init(void);

#endif
