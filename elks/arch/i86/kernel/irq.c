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
 * 9/1999 The 100 Hz system timer 0 can configure for variable input
 *        frequency. Christian Mardm"oller (chm@kdt.de)
 *
 * 4/2001 Use macros to directly generate timer constants based on the 
 *        value of HZ macro. This works the same for both the 8253 and 
 *        8254 timers. - Thomas McWilliams  <tmwo@users.sourceforge.net> 
 */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/timex.h>
#include <linuxmt/timer.h>
#include <linuxmt/hdreg.h>

#include <arch/system.h>
#include <arch/irq.h>
#include <arch/io.h>
#include <arch/keyboard.h>

struct irqaction 
{
	void (*handler)();
	void *dev_id;
};

static struct irqaction irq_action[32];

#ifdef CONFIG_ARCH_SIBO

/*
 *	Low level interrupt handling for the SIBO platform
 */
 
void disable_irq(int nr)
{
	/* Not supported on SIBO */
}

void enable_irq(irq_nr)
unsigned int irq_nr;
{
	/* Not supported on SIBO */
}

static int remap_irq(x)
int x;
{
	return x;
}

static void arch_init_IRQ()
{
}

#else

/*
 *	Low level interrupt handling for the X86 PC/XT and PC/AT
 *	platform
 */
 
unsigned char cache_21 = 0xff;
unsigned char cache_A1 = 0xff;

#if 0
void disable_irq(irq_nr)
unsigned int irq_nr;
{
	flag_t flags;
	unsigned char mask = 1 << (irq_nr & 7);
	save_flags(flags);
	icli();
	if(irq_nr < 8) {
		cache_21 |= mask;
		outb(cache_21,0x21);
	}
	else
	{
		cache_A1 |= 1 << (irq-8);
		outb(cache_A1,0xA1);
	}
	restore_flags(flags);
}
#endif

void enable_irq(irq_nr)
unsigned int irq_nr;
{
	flag_t flags;
	unsigned char mask;

	mask = ~(1 << (irq_nr & 7));
	save_flags(flags);
	if (irq_nr < 8) {
		icli();
		cache_21 &= mask;
		outb(cache_21,0x21);
		restore_flags(flags);
		return;
	}
	icli();
	cache_A1 &= mask;
	outb(cache_A1,0xA1);
	restore_flags(flags);
}


static int remap_irq(irq)
int irq;
{
	if (irq > 15) {
		return -EINVAL;
	}
	if (irq > 7 && arch_cpu<2) {
		return -EINVAL;		/* AT interrupt line on an XT */
	}
	if (irq == 2 && arch_cpu>1)
		irq = 9;		/* Map IRQ 9/2 over */
}

/*
These 8253/8254 macros generate proper timer constants based on the timer
tick macro HZ which is defined in timex.h (usually 100 Hz).

The PC timer chip can be programmed to divide its reference frequency by 
a 16 bit unsigned number. The reference frequency of 1193181.8 Hz happens 
to be 1/3 of the NTSC color burst frequency.  In fact, the hypothetical 
exact reference frequency for the timer is 39375000/33 Hz. The macros
use scaled fixed point arithmetic for greater accuracy. 
*/

#define TIMER_CMDS_PORT 0x43  /* command port */
#define TIMER_DATA_PORT 0x40  /* data port    */

#define TIMER_MODE0 0x30   /* timer 0, binary count, mode 0, lsb/msb */
#define TIMER_MODE2 0x34   /* timer 0, binary count, mode 2, lsb/msb */ 

#define TIMER_LO_BYTE (__u8)(((5+(11931818L/(HZ)))/10)%256)
#define TIMER_HI_BYTE (__u8)(((5+(11931818L/(HZ)))/10)/256)

static void arch_init_IRQ()
{
	/* Stop the timer */
	outb (TIMER_MODE0, TIMER_CMDS_PORT);
	outb (0, TIMER_DATA_PORT);
	outb (0, TIMER_DATA_PORT);
}

static void enable_timer_tick()
{
	/* set the clock frequency */
	outb (TIMER_MODE2, TIMER_CMDS_PORT);   
	outb (TIMER_LO_BYTE, TIMER_DATA_PORT); /* LSB */
	outb (TIMER_HI_BYTE, TIMER_DATA_PORT); /* MSB */
}

#endif


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
	} else {
		if(i > 15) {
			printk("Unexpected trap: %d\n", i-16);
		} else {
			printk("Unexpected interrupt: %d\n", i);
		}
	}
	lastirq = -1;
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


/*unsigned int restore_flags(flags)
{*/
/* this version is smaller than the functionally equivalent C version
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


int request_irq(irq, handler, dev_id)
int irq;
void (*handler);
void *dev_id;
{
	register struct irqaction *action;
	flag_t flags;
	
	irq = remap_irq(irq);
	if(irq < 0)
		return -EINVAL;
		
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

	disable_irq(irq);

	action->handler = NULL;
	action->dev_id = NULL;
	action->flags = 0;
	action->name = NULL;
	restore_flags(flags);
}
#endif

/*
 *	IRQ setup.
 */


void init_IRQ()
{
	register struct irqaction *irq = irq_action + 16;
	int ct;
#ifdef ROM_USE_ORG_INTMASK       /* for example Debugger :-) */
        cache_21 = inb_p(0x21);
#endif

	arch_init_IRQ();

	/* Old IRQ 8 handler is nuked in this routine */
	irqtab_init();			/* Store DS */

	/* Set off the initial timer interrupt handler */
	if (request_irq(0, timer_tick,NULL))
		panic("Unable to get timer");

	/* Re-start the timer only after irq is set */

	enable_timer_tick();
	
#ifndef CONFIG_ARCH_SIBO
	
#ifdef CONFIG_CONSOLE_DIRECT
	/* Set off the initial keyboard interrupt handler */

	if (request_irq(1, keyboard_irq,NULL)) {
		panic("Unable to get keyboard");
	}

#else /* CONFIG_CONSOLE_DIRECT */
	enable_irq(1);		/* Keyboard */
#endif /* CONFIG_CONSOLE_DIRECT */

	/* Enable the drop through interrupts. */

	if(arch_cpu > 1)
	{
		enable_irq(HD_IRQ);	/* AT ST506 */
		enable_irq(15);		/* AHA1542 */
	}
	enable_irq(5);		/* XT ST506 */
	enable_irq(2);		/* Cascade */
	enable_irq(6);		/* Floppy */

#endif /* CONFIG_ARCH_SIBO */
}

