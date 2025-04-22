/*
 * Solo86 Interrupt Handling
 *
 * Ferry Hendrikx, April 2025
 */
#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <arch/io.h>
#include <arch/irq.h>
#include <arch/ports.h>

/*
 *  Low level interrupt handling for the Solo86 platform
 */

void initialize_irq(void)
{
}

void enable_irq(unsigned int irq)
{
    // not required
}

int remap_irq(int irq)
{
    return irq;
}

// Get interrupt vector from IRQ

int irq_vector (int irq)
{
    // IRQ 0-7 are mapped to vectors INT 20h-27h

    return irq + 0x20;
}

void disable_irq(unsigned int irq)
{
    // not required
}
