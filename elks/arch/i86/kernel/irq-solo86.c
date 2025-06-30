/*
 * Interrupt Handling for Solo/86
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
 *  Low level interrupt handling for the Solo/86 platform
 */

void initialize_irq(void)
{
    // disable all IRQs

    outb(0x00, INT_CMDS_PORT);
}

void enable_irq(unsigned int irq)
{
    if (irq > 3)
        return;

    unsigned char mask = 1 << irq;
    unsigned char state = inb(INT_CMDS_PORT);

    state |= mask;

    outb(state, INT_CMDS_PORT);
}

int remap_irq(int irq)
{
    return (irq);
}

// Get interrupt vector from IRQ

int irq_vector (int irq)
{
    // IRQ 0-7 are mapped to vectors INT 20h-27h

    return (irq + 0x20);
}

void disable_irq(unsigned int irq)
{
    if (irq > 3)
        return;

    flag_t flags;

    save_flags(flags);
    clr_irq();

    unsigned char mask = ~(1 << irq);
    unsigned char state = inb(INT_CMDS_PORT);

    state &= mask;

    outb(state, INT_CMDS_PORT);

    restore_flags(flags);
}
