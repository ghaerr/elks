/*
 * 8018X's Integrated Interrupt Controller Unit
 *
 * This file contains code used for the embedded 8018X family only.
 * 
 * 16 May 21 Santiago Hormazabal
 */

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>

#include <arch/ports.h>
#include <arch/io.h>
#include <arch/irq.h>

/*
 *	Low level interrupt handling for the X86 8018X
 *	platforms
 */

void init_irq(void)
{
    /* Mask all interrupt sources */
    outw(0xfd, 0xff00 + 0x08); /* IMASK */
}

void enable_irq(unsigned int irq)
{
    if (irq == TIMER_IRQ) {
        /* 8018x: Timer Interrupt unmasked, priority 7 */
        outw(7, 0xff00 + 0x12); /* TCUCON */
    }
}

int remap_irq(int irq)
{
    return irq;
}

// Get interrupt vector from IRQ

int irq_vector (int irq)
{
    if (irq == TIMER_IRQ) {
        /* 8018x: Timer 1 Interrupt => Interrupt type (vector) 18 */
        return 18;
    }
    return -EINVAL;
}
