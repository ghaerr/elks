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

    init_irq();

    disable_timer_tick();

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
