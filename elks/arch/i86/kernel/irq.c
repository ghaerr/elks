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
 */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/timex.h>

#include <arch/system.h>
#include <arch/irq.h>
#include <arch/io.h>
#include <arch/keyboard.h>

struct irqaction 
{
	void (*handler)();
	unsigned long flags;
/*	unsigned long mask; */ /* Mask unused so removed to save space */
	char *name;
};

#define IRQF_BAD	1		/* This IRQ is bad if it occurs */
	
static struct irqaction irq_action[32]= { 
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
	{NULL,IRQF_BAD,NULL},
};

unsigned char cache_21 = 0xff;
unsigned char cache_A1 = 0xff;

#if 0
void disable_irq(irq_nr)
unsigned int irq_nr;
{
	unsigned int flags;
	unsigned char mask = 1 << (irq_nr & 7);
	save_flags(flags);
	if(irq_nr < 8) {
		cli();
		cache_21 |= mask;
		outb(cache_21,0x21);
		restore_flags(flags);
		return;
	}
	cli();
/* BNOTE Is there missing code here! ????? cache_A1 |= mask ????? */
	outb(cache_A1,0xA1);
	restore_flags(flags);
}
#endif

void enable_irq(irq_nr)
unsigned int irq_nr;
{
	unsigned int flags;
	unsigned char mask;

	mask = ~(1 << (irq_nr & 7));
	save_flags(flags);
#ifndef CONFIG_XT
	if (irq_nr < 8) {
#endif
		cli();
		cache_21 &= mask;
		outb(cache_21,0x21);
		restore_flags(flags);
		return;
#ifndef CONFIG_XT
	}
	cli();
	cache_A1 &= mask;
	outb(cache_A1,0xA1);
	restore_flags(flags);
#endif
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
	if(irq->flags&IRQF_BAD) {
		if(i > 15) {
			printk("Unexpected trap: %d\n", i-16);
		} else {
			printk("Unexpected interrupt: %d\n", i);
		}
	} else if(irq->handler != NULL) {
		irq->handler(i,regs);
		lastirq = -1;
	}
}

/*
 *	Low level IRQ control.
 */
 
/*unsigned long __save_flags()
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
	unsigned int v=flags;
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

void cli()
{
#asm
	cli
#endasm
}

void sti()
{
#asm
	sti
#endasm
}
		
int request_irq(irq, handler, irqflags, devname)
int irq;
void (*handler);
unsigned long irqflags;
char *devname;	
{
	register struct irqaction *action;
	unsigned int flags;
#ifdef CONFIG_XT
	if (irq > 7) {
		return -EINVAL;
	}
#else	
	if (irq > 15) {
		return -EINVAL;
	}
	if (irq > 7 && arch_cpu<2) {
		return -EINVAL;		/* AT interrupt line on an XT */
	}
	if (irq == 2 && arch_cpu>1)
		irq = 9;		/* Map IRQ 9/2 over */
#endif
	action = irq_action + irq;
	if(action->handler) {
		return -EBUSY;
	}
	if(!handler) {
		return -EINVAL;
	}
	save_flags(flags);
	cli();
	action->handler = handler;
	action->flags = irqflags;
/*	action->mask = 0; */ /* Mask unused, so removed to save space */
	action->name = devname;
	enable_irq(irq);
	restore_flags(flags);
	return 0;
}

#if 0
void free_irq(irq)
unsigned int irq;
{
	register struct irqaction * action = irq_action + irq;
	unsigned int flags;

	if (irq > 15) {
		printk("Trying to free IRQ%d\n",irq);
		return;
	}
	if (!action->handler) {
		printk("Trying to free free IRQ%d\n",irq);
		return;
	}
	save_flags(flags);
	cli();
	if (irq < 8) {
		cache_21 |= 1 << irq;
		outb(0x21,cache_21);
	} else {
		cache_A1 |= 1 << (irq-8);
		outb(0xA1,cache_A1);
	}
	action->handler = NULL;
	action->flags = 0;
/*	action->mask = 0; */ /* Mask unused so removed to save space */
	action->name = NULL;
	restore_flags(flags);
}
#endif

/*
 *	Test timer tick routine
 */

unsigned long jiffies=0;

void timer_tick(/*struct pt_regs * regs*/)
{
	do_timer(/*regs*/);
#ifdef NEED_RESCHED	/* need_resched is not checked anywhere */
	if (((int)jiffies & 7) == 0)
		need_resched=1;	/* how primitive can you get? */
#endif
#if 0
	#asm
	! rotate the 20th character on the 3rd screen line
	push es
	mov ax,#0xb80a
	mov es,ax
	seg es
	inc 40
	pop es
	#endasm
#endif
}

/*
 *	IRQ setup.
 *
 */
 
void init_IRQ()
{
	register struct irqaction *irq = irq_action + 16;
	int ct;

#if 1
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
	 
	if(request_irq(0, timer_tick, 0L, "timer"))
		panic("Unable to get timer");

	/*
	 *	Re-start the timer only after irq is set
	 */
 
#if 1
	/* set the clock to 100 Hz */
	outb_p(0x34,0x43);		/* binary, mode 2, LSB/MSB, ch 0 */
	outb_p(156,0x40);		/* LSB */
	outb_p(46,0x40);		/* MSB */
#endif

	/*
	 *	Set off the initial keyboard interrupt handler
	 */

	if(request_irq(1, keyboard_irq, 0L, "keyboard"))
		panic("Unable to get keyboard");
	/*
	 *	Enable the drop through interrupts.
	 */
#ifndef CONFIG_XT	 	
	enable_irq(14);		/* AT ST506 */
	enable_irq(15);		/* AHA1542 */
#endif
	enable_irq(5);		/* XT ST506 */
	enable_irq(2);		/* Cascade */
	enable_irq(6);		/* Floppy */

	/*
	 *	All traps are bad initially. One day IRQ's will be.
	 */
#if 0	/* This is now done in the declaration at the top. */
	for(ct=16;ct<31;ct++)
		irq++->flags|=IRQF_BAD;
#endif
}

