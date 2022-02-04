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
#include <linuxmt/heap.h>

#include <arch/ports.h>
#include <arch/segment.h>
#include <arch/irq.h>


static irq_handler irq_action [16];

/*
 *	Called by the assembler hooks
 */

void do_IRQ(int i,void *regs)
{
    register irq_handler ih = irq_action [i];
    if (!ih) {
        printk("Unexpected interrupt: %u\n", i);
        return;
    }
    (*ih)(i, regs);
}


// TODO: simplify the whole by replacing IRQ by INT number
// Would also allow to handle any of the 0..255 interrupts
// including the 0..7 processor exceptions & traps

struct int_handler {
	byte_t call;		/* CALLF opcode (9Ah) */
	int_proc proc;
	word_t seg;
	byte_t irq;
} __attribute__ ((packed));

typedef struct int_handler int_handler_s;


// Add a dynamically allocated handler
// that redirects to the static handler

static int int_handler_add (int irq, int vect, int_proc proc)
{
	int_handler_s * h = (int_handler_s *) heap_alloc (sizeof (int_handler_s), HEAP_TAG_INTHAND);
	if (!h) return -ENOMEM;

	h->call = 0x9A;		/* CALLF opcode */
	h->proc = proc;
	h->seg  = kernel_cs;	/* resident kernel code segment */
	h->irq  = irq;

	int_vector_set (vect, (int_proc) h, kernel_ds);

	return 0;
}


int request_irq(int irq, irq_handler handler, int hflag)
{
    flag_t flags;

    irq = remap_irq(irq);
    if (irq < 0 || !handler) return -EINVAL;

    if (irq_action [irq]) return -EBUSY;

    save_flags(flags);
    clr_irq();

    irq_action [irq] = handler;

    int_proc proc;
    if (hflag == INT_SPECIFIC)
	proc = (int_proc) handler;
    else
	proc = _irqit;

    // TODO: IRQ number has no meaning for an INT handler
    // see above simplification TODO
    if (int_handler_add (irq, irq_vector (irq), proc))
	return -ENOMEM;

    enable_irq(irq);

    restore_flags(flags);

    return 0;
}

#if 0
void free_irq(unsigned int irq)
{
    flag_t flags;

    if (irq > 15) {
	printk("Trying to free IRQ%u\n",irq);
	return;
    }
    if (!irq_action [irq]) {
	printk("Trying to free free IRQ%u\n",irq);
	return;
    }
    save_flags(flags);
    clr_irq();

    disable_irq(irq);

    irq_action [irq] = NULL;

    restore_flags(flags);
}
#endif

/*
 *	IRQ setup.
 */

void INITPROC irq_init(void)
{
    init_irq();		/* PIC initialization */

    // TODO: no more need to disable the timer
    // BEFORE the interrupt initialization
    // as timer vector is kept as is

    // TODO: move that timer stuff in `timer.c`
    // to clearly separate the PIC and PIT driving
    disable_timer_tick();

    irqtab_init();	/* now only saves previous vector 08h (timer) */
    int_handler_add (0x80, 0x80, _irqit); /* system call */

    /* Set off the initial timer interrupt handler */
    if (request_irq(TIMER_IRQ, timer_tick, INT_GENERIC))
    	panic("Unable to get timer");

    /* Re-start the timer */
    enable_timer_tick();
}
