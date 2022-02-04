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
#include <arch/8018x.h>
#include <arch/io.h>
#include <arch/irq.h>

/*
 *	Low level interrupt handling for the X86 8018X
 *	platforms
 */

void init_irq(void)
{
    /* Mask all interrupt sources on the IMASK register */
    outw(0xfd, PCB_IMASK);
}

void enable_irq(unsigned int irq)
{
    if (irq == TIMER_IRQ) {
        /* 8018x: Timer Interrupt unmasked, priority 7. */
        outw(7, PCB_TCUCON);
    } else if ((irq == UART0_IRQ_RX) || (irq == UART0_IRQ_TX)) {
        /**
         * 8018x: Enabling Serial Interrupts will enable the RX and TX IRQs
         * at the same time, this is a CPU limitation.
         * Serial Interrupts unmasked, priority 1.
         */
        outw(1, PCB_SCUCON);
    }
}

int remap_irq(int irq)
{
    /* no remaps */
    return irq;
}

// Get interrupt vector from IRQ
int irq_vector(int irq)
{
    if (irq == TIMER_IRQ) {
        /* 8018x: Timer 1 Interrupt => Interrupt type (vector) 18 */
        return CPU_VEC_TIMER1;
    } else if (irq == UART0_IRQ_RX) {
        /* 8018x: Serial 0 RX Interrupt => Interrupt type (vector) 20 */
        return CPU_VEC_S0_RX;
    } else if (irq == UART0_IRQ_TX) {
        /* 8018x: Serial 0 TX Interrupt => Interrupt type (vector) 21 */
        return CPU_VEC_S0_TX;
    }
    return -EINVAL;
}
