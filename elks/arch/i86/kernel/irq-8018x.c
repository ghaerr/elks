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

#include <arch/io.h>
#include <arch/irq.h>

/* TMR and SER interrupts are enabled by default */
unsigned int cache_08 = 0xfa;

/*
 *	Low level interrupt handling for the X86 8018X
 *	platforms
 */

void init_irq(void)
{
}

void enable_irq(unsigned int irq)
{
    unsigned int mask;

    mask = ~(1 << irq);

    cache_08 &= mask;

    /* TODO: Create a list of PCB registers instead of using hardcoded offsets */
    outw(cache_08, 0xff00 + 0x08); /* IMASK */
}

int remap_irq(int irq)
{
    return irq;
}

// Get interrupt vector from IRQ

int irq_vector (int irq)
	{
	// IRQ 0-7  are mapped to vectors INT 08h-0Fh

	return irq + 0x08;
	}
