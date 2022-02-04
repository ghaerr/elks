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
    struct timer_list *tl_prev;
    jiff_t tl_expires;
    int tl_data;
    void (*tl_function) ();
};

struct pt_regs;

/* sched.c*/
extern void init_timer(struct timer_list *);
extern void add_timer(struct timer_list *);
extern int del_timer(struct timer_list *);
extern void do_timer(struct pt_regs *);

/* timer.c*/
void timer_tick(int, struct pt_regs *);
void spin_timer(int);

/* timer-8254.c*/
void enable_timer_tick(void);
void disable_timer_tick(void);

#endif
