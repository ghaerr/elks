/*
 * WonderSwan interrupt control
 */
#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <arch/io.h>
#include <arch/irq.h>
#include <arch/ports.h>
#include <arch/swan.h>

/*
 *  Low level interrupt handling for the WonderSwan platform
 */

void initialize_irq(void)
{
    outb(0x08, IRQ_VECTOR_PORT);
    outb(0x00, IRQ_ENABLE_PORT);
    outb(0xFF, IRQ_ACK_PORT);
}

void enable_irq(unsigned int irq)
{
    unsigned char mask = 1 << (irq & 7);

    if (irq >= 8 && irq < 16)
        outb(inb(IRQ_ENABLE_PORT) | mask, IRQ_ENABLE_PORT);
}

int remap_irq(int irq)
{
    if ((unsigned int)irq > 15)
        return -EINVAL;
    return irq;
}

int irq_vector (int irq)
{
    return irq;
}

void disable_irq(unsigned int irq)
{
    flag_t flags;
    unsigned char mask = 1 << (irq & 7);

    save_flags(flags);
    clr_irq();
    if (irq >= 8 && irq < 16)
        outb(inb(IRQ_ENABLE_PORT) & ~mask, IRQ_ENABLE_PORT);
    restore_flags(flags);
}
