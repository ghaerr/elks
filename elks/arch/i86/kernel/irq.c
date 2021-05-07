/*
 *  linux/arch/i86/kernel/irq.c
 *
 *  Copyright (C) 1992 Linus Torvalds
 *  Modified for Linux/8086 by Alan Cox Nov 1995.
 *
 * This file contains the code used by various IRQ handling routines:
 * asking for different IRQ's should be done through these routines
 * instead of just grabbing them. Thus setups with different IRQ numbers
 * shouldn't result in any weird surprises, and installing new handlers
 * should be easier.
 *
 * 9/1999 For ROM debuggers you can configure that not all interrupts
 *        disable.
 */

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/init.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/timer.h>
#include <linuxmt/types.h>

#include <arch/io.h>
#include <arch/irq.h>
#include <arch/ports.h>

/* Enable 80386 Traps */
/*#define ENABLE_TRAPS */

struct irqaction {
	void (*handler)();
	void *dev_id;
};

static void default_handler(int i, void *regs, void *dev);

static struct irqaction irq_action[] = {
    {default_handler, NULL}, {default_handler, NULL}, {default_handler, NULL},
    {default_handler, NULL}, {default_handler, NULL}, {default_handler, NULL},
    {default_handler, NULL}, {default_handler, NULL}, {default_handler, NULL},
    {default_handler, NULL}, {default_handler, NULL}, {default_handler, NULL},
    {default_handler, NULL}, {default_handler, NULL}, {default_handler, NULL},
    {default_handler, NULL},
#ifdef ENABLE_TRAPS
    {default_handler, NULL}, {default_handler, NULL}, {default_handler, NULL},
    {default_handler, NULL}, {default_handler, NULL}, {default_handler, NULL},
    {default_handler, NULL}, {default_handler, NULL}, {default_handler, NULL},
    {default_handler, NULL}, {default_handler, NULL}, {default_handler, NULL},
    {default_handler, NULL}, {default_handler, NULL}, {default_handler, NULL},
    {default_handler, NULL},
#endif
};

unsigned char cache_21 = 0xff, cache_A1 = 0xff;

#ifdef CONFIG_ARCH_SIBO
void enable_irq(unsigned int irq)
{
    /* Not supported on SIBO */
}

static int remap_irq(unsigned int irq)
{
    return irq;
}

#else

/*
 *	Low level interrupt handling for the X86 PC/XT and PC/AT
 *	platform
 */

void enable_irq(unsigned int irq)
{
    unsigned char mask;

    mask = ~(1 << (irq & 7));
    if (irq < 8) {
	cache_21 &= mask;
	outb(cache_21,((void *) 0x21));
    } else {
	cache_A1 &= mask;
	outb(cache_A1,((void *) 0xA1));
    }
}

static int remap_irq(int irq)
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

#endif

/*
 *	Called by the assembler hooks
 */

void do_IRQ(int i,void *regs)
{
    register struct irqaction *irq = irq_action + i;

    irq->handler(i, regs, irq->dev_id);
}

static void default_handler(int i, void *regs, void *dev)
{
#ifdef ENABLE_TRAPS
    if (i > 15)
	printk("Unexpected trap: %u\n", i-16);
    else
#endif
	printk("Unexpected interrupt: %u\n", i);
}

int request_irq(int irq, void (*handler)(int,struct pt_regs *,void *), void *dev_id)
{
    register struct irqaction *action;
    flag_t flags;

    irq = remap_irq(irq);
    if (irq < 0)
	return -EINVAL;

    action = irq_action + irq;
    if (action->handler != default_handler)
	return -EBUSY;

    if (!handler)
	return -EINVAL;

    save_flags(flags);
    clr_irq();

    action->handler = handler;
    action->dev_id = dev_id;

    enable_irq((unsigned int) irq);

    restore_flags(flags);

    return 0;
}

#if 0

void free_irq(unsigned int irq)
{
    register struct irqaction * action = irq_action + irq;
    flag_t flags;

    if (irq > 15) {
	printk("Trying to free IRQ%u\n",irq);
	return;
    }
    if (action->handler == default_handler) {
	printk("Trying to free free IRQ%u\n",irq);
	return;
    }
    save_flags(flags);
    clr_irq();

    disable_irq(irq);

    action->handler = default_handler;
    action->dev_id = NULL;
/*    action->flags = 0;
    action->name = NULL;*/

    restore_flags(flags);
}

#endif

/*
 *	IRQ setup.
 */

void INITPROC irq_init(void)
{
    flag_t flags;

#ifdef CONFIG_HW_259_USE_ORIGINAL_MASK	/* for example Debugger :-) */
    cache_21 = inb_p(0x21);
#endif

    stop_timer();

    /* Old IRQ 8 handler is nuked in this routine */
    irqtab_init();			/* Store DS */

    /* Set off the initial timer interrupt handler */
    if (request_irq(TIMER_IRQ, timer_tick, NULL))
	panic("Unable to get timer");

    /* Re-start the timer only after irq is set */
    enable_timer_tick();

    if (sys_caps & CAP_IRQ2MAP9) {	/* PC/AT or greater */
	save_flags(flags);
	clr_irq();
	enable_irq(2);		/* Cascade slave PIC */
	restore_flags(flags);
    }
}
