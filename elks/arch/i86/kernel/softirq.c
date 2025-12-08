#include <linuxmt/config.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <arch/irq.h>

/*
 * Bottom half interrupt handler support (softirq's)
 *
 * Top half interrupt handlers should quickly perform hardware data acquisition,
 * then call mark_bh to request deferred execution of the bottom half handler
 * registered with init_bh, for non-interrupt-priority tasks like wake_up, etc.
 *
 * All BH handlers run in interrupt context, which means they can't sleep,
 * can't access user space through current, and can't reschedule.
 *
 * Interrupts are enabled and a BH handler may be interrupted by any hardware
 * interrupt, but are protected from being re-entered, so don't need to be reentrant.
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
