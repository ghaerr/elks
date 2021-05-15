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

/* 8259 Command/Data ports */
#define PIC1_CMD   0x20   /* master */
#define PIC1_DATA  0x21
#define PIC2_CMD   0xA0   /* slave */
#define PIC2_DATA  0xA1

static unsigned char cache_21 = 0xff, cache_A1 = 0xff;

/*
 *	Low level interrupt handling for the X86 PC/XT and PC/AT platform
 */

void init_irq(void)
{
#ifdef CONFIG_HW_259_USE_ORIGINAL_MASK	/* for example Debugger :-) */
    cache_21 = inb_p(PIC1_DATA);
#endif
}

void enable_irq(unsigned int irq)
{
    unsigned char mask;

    mask = ~(1 << (irq & 7));
    if (irq < 8) {
	cache_21 &= mask;
	outb(cache_21, PIC1_DATA);
    } else {
	cache_A1 &= mask;
	outb(cache_A1, PIC2_DATA);
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

// Get interrupt vector from IRQ

int irq_vector (int irq)
	{
	// IRQ 0-7  are mapped to vectors INT 08h-0Fh
	// IRQ 8-15 are mapped to vectors INT 70h-77h

	return irq + ((irq > 8) ? 0x68 : 0x08);
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
	outb(cache_21, PIC1_DATA);
    } else {
	cache_A1 |= mask;
	outb(cache_A1, PIC2_DATA);
    }
    restore_flags(flags);
}
#endif
