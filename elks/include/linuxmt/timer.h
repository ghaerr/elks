#ifndef __LINUXMT_TIMER_H
#define __LINUXMT_TIMER_H

#include <linuxmt/types.h>

/*
 * "New and improved" way of handling timers more dynamically.
 * Hopefully efficient and general enough for most things.
 *
 * The "hardcoded" timers above are still useful for well-
 * defined problems, but the timer-list is probably better
 * when you need multiple outstanding timers or similar.
 *
 * The "data" field is in case you want to use the same
 * timeout function for several timeouts. You can use this
 * to distinguish between the different invocations.
 */
struct timer_list {
    struct timer_list *tl_next;
    jiff_t tl_expires;
    int tl_data;
    void (*tl_function) ();
};

/* sched.c*/
void add_timer(struct timer_list *);
int del_timer(struct timer_list *);
void run_timer_list(void);

/* timer.c*/
void timer_tick(int, struct pt_regs *);
void timer_bh(void);
void spin_timer(int);

/* timer-8254.c*/
void enable_timer_tick(void);
void disable_timer_tick(void);

extern jiff_t uptime;       /* uptime for CONFIG_CPU_USAGE */

#endif
