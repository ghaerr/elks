#include <linuxmt/config.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <arch/irq.h>

/*
 * Run bottom-half interrupt handlers
 *
 * 6 Dec 25 Greg Haerr
 */

unsigned int bh_active;
void (*bh_base[MAX_SOFTIRQ])(void);

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
