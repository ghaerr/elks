/*
 *	linux/arch/i86/kernel/irq.c
 *
 *	Copyright (C) 1992 Linus Torvalds
 *	Modified for Linux/8086 by Alan Cox Nov 1995.
 *
 * This file contains the code used by various IRQ handling routines:
 * asking for different IRQ's should be done through these routines
 * instead of just grabbing them. Thus setups with different IRQ numbers
 * shouldn't result in any weird surprises, and installing new handlers
 * should be easier.
 *
 * 9/1999 For ROM debuggers you can configure that not all interrupts
 *        disable.
 * 9/1999 The 100 Hz system timer 0 can configure for variable input
 *        frequentcy. Christian Mardm"oller (chm@kdt.de)
 */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/timex.h>
#include <linuxmt/hdreg.h>

#include <arch/system.h>
#include <arch/irq.h>
#include <arch/io.h>
#include <arch/keyboard.h>

extern void timer_tick();
extern void enable_timer_tick();


struct irqaction 
{
	void (*handler)();
/*	unsigned long mask; */ /* Mask unused so removed to save space */
	void *dev_id;
};

#define IRQF_BAD	1		/* This IRQ is bad if it occurs */
	
static struct irqaction irq_action[32];

unsigned char cache_21 = 0xff;
unsigned char cache_A1 = 0xff;

#if 0
void disable_irq(irq_nr)
unsigned int irq_nr;
{
#ifndef CONFIG_ARCH_SIBO
	flag_t flags;
	unsigned char mask = 1 << (irq_nr & 7);
	save_flags(flags);
	if(irq_nr < 8) {
		icli();
		cache_21 |= mask;
		outb(cache_21,0x21);
		restore_flags(flags);
		return;
	}
	icli();
/* BNOTE Is there missing code here! ????? cache_A1 |= mask ????? */
	outb(cache_A1,0xA1);
	restore_flags(flags);
#endif
}
#endif

void enable_irq(irq_nr)
unsigned int irq_nr;
{
#ifndef CONFIG_ARCH_SIBO
	flag_t flags;
	unsigned char mask;

	mask = ~(1 << (irq_nr & 7));
	save_flags(flags);
#ifndef CONFIG_ARCH_PC_XT
	if (irq_nr < 8) {
#endif
		icli();
		cache_21 &= mask;
		outb(cache_21,0x21);
		restore_flags(flags);
		return;
#ifndef CONFIG_ARCH_PC_XT
	}
	icli();
	cache_A1 &= mask;
	outb(cache_A1,0xA1);
	restore_flags(flags);
#endif
#endif /* CONFIG_ARCH_SIBO */
}

/*
 *	Called by the assembler hooks
 */

int lastirq;
 
void do_IRQ(i, regs)
int i;
void *regs;
{
	register struct irqaction *irq = irq_action + i;

	lastirq = i;	

	if (irq->handler != NULL) {
		irq->handler(i,regs,irq->dev_id);
		lastirq = -1;
	} else {
		if(i > 15) {
			printk("Unexpected trap: %d\n", i-16);
		} else {
			printk("Unexpected interrupt: %d\n", i);
		}
	}
}

/*
 *	Low level IRQ control.
 */
 
/*unsigned int __save_flags()
{*/
#asm
	.globl ___save_flags
	.text
___save_flags:
	pushf
	pop ax
	ret
#endasm
/*}*/

#if 0 /* this version leaves something to be desired. */
void restore_flags(flags)
unsigned int flags;
{
	flag_t v=flags;
#asm
	push ax
	popf
#endasm
}
#else
/* this version is smaller than the functionally equivalent C version above
   at 7 bytes vs. 21 or thereabouts :-) --Alastair Bridgewater */
#asm
        .globl _restore_flags
	.text
_restore_flags:
	pop ax
	pop cx
	push cx
	push cx
	popf
	jmp ax
#endasm
#endif

#if 0
void icli()
{
#asm
	cli
#endasm
}

void isti()
{
#asm
	sti
#endasm
}
#endif

		
int request_irq(irq, handler, dev_id)
int irq;
void (*handler);
void *dev_id;
{
	register struct irqaction *action;
	flag_t flags;
#ifndef CONFIG_ARCH_SIBO
#ifdef CONFIG_ARCH_PC_XT
	if (irq > 7) {
		return -EINVAL;
	}
#else /* CONFIG_ARCH_PC_XT */
	if (irq > 15) {
		return -EINVAL;
	}
	if (irq > 7 && arch_cpu<2) {
		return -EINVAL;		/* AT interrupt line on an XT */
	}
	if (irq == 2 && arch_cpu>1)
		irq = 9;		/* Map IRQ 9/2 over */
#endif /* CONFIG_ARCH_PC_XT */
#endif /* CONFIG_ARCH_SIBO */
	action = irq_action + irq;
	if(action->handler) {
		return -EBUSY;
	}
	if(!handler) {
		return -EINVAL;
	}
	save_flags(flags);
	icli();
	action->handler = handler;
	action->dev_id = dev_id;
/*	action->mask = 0; */ /* Mask unused, so removed to save space */
/*	action->name = devname; */ /* name unsed - ditto */
	enable_irq(irq);
	restore_flags(flags);
	return 0;
}

#if 0
void free_irq(irq)
unsigned int irq;
{
	register struct irqaction * action = irq_action + irq;
	flag_t flags;

	if (irq > 15) {
		printk("Trying to free IRQ%d\n",irq);
		return;
	}
	if (!action->handler) {
		printk("Trying to free free IRQ%d\n",irq);
		return;
	}
	save_flags(flags);
	icli();
	if (irq < 8) {
		cache_21 |= 1 << irq;
		outb(cache_21,0x21);
	} else {
		cache_A1 |= 1 << (irq-8);
		outb(cache_A1,0xA1);
	}
	action->handler = NULL;
	action->dev_id = NULL;
	action->flags = 0;
/*	action->mask = 0; */ /* Mask unused so removed to save space */
	action->name = NULL;
	restore_flags(flags);
}
#endif

/*
 *	IRQ setup.
 *
 */

          
void init_IRQ()
{
	register struct irqaction *irq = irq_action + 16;
	int ct;
#ifdef ROM_USE_ORG_INTMASK       /* for example Debugger :-) */
        cache_21 = inb_p(0x21);
#endif

#ifndef CONFIG_ARCH_SIBO
	/* Stop the timer */
	outb_p(0x30,0x43);
	outb_p(0,0x40);
	outb_p(0,0x40);
#endif

	/* Old IRQ 8 handler is nuked in this routine */
	irqtab_init();			/* Store DS */

	/*
	 *	Set off the initial timer interrupt handler
	 */
	 
	if(request_irq(0, timer_tick,NULL))
		panic("Unable to get timer");

	/*
	 *	Re-start the timer only after irq is set
	 */
 
#ifdef CONFIG_ARCH_SIBO
	enable_timer_tick();
#else /* CONFIG_ARCH_SIBO */
	/* set the clock to 100 Hz */
	outb_p(0x36,0x43);		/* binary, mode 2, LSB/MSB, ch 0 */
#ifdef ROM_8253_100HZ
	outb_p(ROM_8253_100HZ&0xff,0x40);	/* LSB */
	outb_p(ROM_8253_100HZ>>8,0x40);		/* MSB */
#else
	outb_p(156,0x40);		/* LSB */
	outb_p(46,0x40);		/* MSB */
#endif

#ifdef CONFIG_CONSOLE_DIRECT

	/*
	 *	Set off the initial keyboard interrupt handler
	 */

	if (request_irq(1, keyboard_irq,NULL)) {
		panic("Unable to get keyboard");
	}

#else /* CONFIG_CONSOLE_DIRECT */
	enable_irq(1);		/* Cascade */
#endif /* CONFIG_CONSOLE_DIRECT */

	/*
	 *	Enable the drop through interrupts.
	 */
#ifndef CONFIG_ARCH_PC_XT	 	
	enable_irq(HD_IRQ);	/* AT ST506 */
	enable_irq(15);		/* AHA1542 */
#endif
	enable_irq(5);		/* XT ST506 */
	enable_irq(2);		/* Cascade */
	enable_irq(6);		/* Floppy */

#endif /* CONFIG_ARCH_SIBO */
}

