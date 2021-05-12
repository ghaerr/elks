/*
 * 8259 Programmable Interrupt Controller
 *
 * This file contains code used for the 8259 PIC only.
 */

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>

#include <arch/io.h>
#include <arch/irq.h>

static unsigned char cache_21 = 0xff, cache_A1 = 0xff;

/*
 *	Low level interrupt handling for the X86 PC/XT and PC/AT platform
 */

void init_irq(void)
{
#ifdef CONFIG_HW_259_USE_ORIGINAL_MASK	/* for example Debugger :-) */
    cache_21 = inb_p(0x21);
#endif
}

void enable_irq(unsigned int irq)
{
    unsigned char mask;

    mask = ~(1 << (irq & 7));
    if (irq < 8) {
	cache_21 &= mask;
	outb(cache_21,((void *) 0x21)); //FIXME
    } else {
	cache_A1 &= mask;
	outb(cache_A1,((void *) 0xA1));
    }
}

int remap_irq(int irq)
{
    if ((unsigned int)irq > 15 || (irq > 7 && !(sys_caps & CAP_IRQ8TO15)))
	return -EINVAL;
    if (irq == 2 && (sys_caps & CAP_IRQ2MAP9))
	irq = 9;			/* Map IRQ 9/2 over */
    return irq;
}

#if 0
void disable_irq(unsigned int irq)
{
    flag_t flags;
    unsigned char mask = 1 << (irq & 7);

    save_flags(flags);
    clr_irq();
    if (irq < 8) {
	cache_21 |= mask;
	outb(cache_21,((void *) 0x21));
    } else {
	cache_A1 |= mask;
	outb(cache_A1,((void *) 0xA1));
    }
    restore_flags(flags);
}
#endif
