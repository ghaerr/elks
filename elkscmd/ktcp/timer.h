#ifndef TIMER_H
#define TIMER_H

#define TIME_LT(a,b)		((long)((a)-(b)) < 0)
#define TIME_LEQ(a,b)		((long)((a)-(b)) <= 0)
#define TIME_GT(a,b)		((long)((a)-(b)) > 0)
#define TIME_GEQ(a,b)		((long)((a)-(b)) >= 0)

typedef	unsigned long timeq_t;
extern timeq_t Now;

int timer_init(void);
timeq_t get_time(void);

#endif
