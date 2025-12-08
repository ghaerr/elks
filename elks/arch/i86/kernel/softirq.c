#include <linuxmt/config.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <arch/irq.h>

/*
 * Bottom half interrupt handler support (softirq's)
 *
 * Top half interrupt handlers should quickly perform hardware data acquisition or
 * extremely high-priority tasks, then call mark_bh to request deferred execution of a
 * bottom half handler registered with init_bh for lesser-priority tasks like wake_up,
 * etc. that can be performed after the end of hardware interrupt is sent to the PIC.
 *
 * BH handlers run with interrupts enabled in an interrupt context similar to the
 * top half, but while TH handlers run before the PIC EOI is sent, BH handlers
 * execute afterwards, thus allowing further PIC interrupts of any priority to run.
 * A BH handler may be interrupted by any top-half interrupt, but are protected
 * from being re-entered, so they don't need to be reentrant.
 *
 * Since BH handlers run in an interrupt context, they can't sleep, can't access user
 * space, and can't reschedule.
 *
 * 6 Dec 25 Greg Haerr
 */

unsigned int bh_active;             /* bitmask of handlers flagged to run by mark_bh */
void (*bh_base[MAX_SOFTIRQ])(void); /* registered handlers from init_bh */

/* run any bottom half handlers flagged in bh_active */
void do_bottom_half(void)
{
    unsigned int active;
    unsigned int mask, left;
    void (**bh)(void);

#if DEBUG
    /* running on interrupt or kernel stack? - may run on either */
    if (getsp() >= endistack && getsp() < istack)
        printk("I");                /* called from idle task */
    else printk("K");               /* called after syscall or user mode interrupt */
    printk("{B%d}", intr_count);
#endif

    bh = bh_base;
    active = bh_active;
    for (mask = 1, left = ~0; left & active; bh++, mask += mask, left += left) {
        if (mask & active) {
            bh_active &= ~mask;
            if (*bh)
                (*bh)();
        }
    }
}
