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

// TODO: simplify the whole by replacing IRQ by INT number
// Would also allow to handle any of the 0..255 interrupts
// including the 0..7 processor exceptions & traps

struct int_handler {
    byte_t call;        /* CALLF opcode (9Ah) */
    int_proc proc;
    word_t seg;
    byte_t irq;
} __attribute__ ((packed));

#define NR_TRAMPOLINES  12
static struct int_handler trampoline [NR_TRAMPOLINES];
static struct int_handler *irq_trampoline [16];
static irq_handler irq_action [16];

/* called by _irqit assembler hook after saving registers */
void do_IRQ(int i,void *regs)
{
    irq_handler ih = irq_action [i];
    if (!ih)
        printk("Unexpected interrupt: %u\n", i);
    else (*ih)(i, regs);
}

static struct int_handler *handler_alloc(void)
{
    struct int_handler *h;

    for (h = trampoline; h < &trampoline[NR_TRAMPOLINES]; h++) {
        if (h->call == 0)
            return h;
    }
    return NULL;
}

static void handler_free(struct int_handler *h)
{
    h->call = 0;
}

/* install interrupt vector to point to allocated handler trampoline */
static int int_handler_add (int irq, int vect, int_proc proc, struct int_handler *h)
{
    if (!h) h = handler_alloc();
    if (!h) return -ENOMEM;

    h->call = 0x9A;         /* CALLF opcode */
    h->proc = proc;
    h->seg  = kernel_cs;    /* resident kernel code segment */
    h->irq  = irq;

    int_vector_set (vect, (int_proc) h, kernel_ds);

    return 0;
}

int request_irq(int irq, irq_handler handler, int hflag)
{
    struct int_handler *h;
    int_proc proc;
    flag_t flags;

    irq = remap_irq(irq);
    if (irq < 0 || !handler) return -EINVAL;

    if (irq_action [irq]) return -EBUSY;
    h = handler_alloc();
    if (!h) return -ENOMEM;

    save_flags(flags);
    clr_irq();

    irq_action [irq] = handler;
    irq_trampoline [irq] = h;

    if (hflag == INT_SPECIFIC)
        proc = (int_proc) handler;
    else
        proc = _irqit;

    // TODO: IRQ number has no meaning for an INT handler
    // see above simplification TODO
    int_handler_add (irq, irq_vector (irq), proc, h);

    enable_irq(irq);
    restore_flags(flags);

    return 0;
}

int free_irq(int irq)
{
    flag_t flags;

    irq = remap_irq(irq);
    if (irq < 0) return -EINVAL;

    if (!irq_action[irq]) {
        printk("Trying to free free IRQ%u\n",irq);
        return -EINVAL;
    }
    save_flags(flags);
    clr_irq();

    disable_irq(irq);
    int_vector_set(irq_vector(irq), 0, 0);  /* reset vector to 0:0 */
    irq_action[irq] = NULL;
    restore_flags(flags);

    handler_free(irq_trampoline[irq]);
    return 0;
}

/*
 *  IRQ setup.
 */
void INITPROC irq_init(void)
{
    /* use INT 0x80h for system calls */
    int_handler_add(0x80, 0x80, _irqit, NULL);

#if defined(CONFIG_TIMER_INT0F) || defined(CONFIG_TIMER_INT1C)
    /* Use IRQ 7 vector (simulated by INT 0Fh) for timer interrupt handler */
    if (request_irq(7, timer_tick, INT_GENERIC))
        panic("Unable to get timer");

#if defined(CONFIG_TIMER_INT1C)
    /* Also map BIOS INT 1C user timer callout to INT 0Fh */
    unsigned long __far *vec1C = _MK_FP(0, 0x1C << 2);  /* BIOS user timer callout */
    unsigned long __far *vec0F = _MK_FP(0, 0x0F << 2);  /* IRQ 7 vector (INT 0Fh) */
    clr_irq();
    *vec1C = *vec0F;            /* point INT 1C to INT 0F vector */
    set_irq();
#endif

#else /* normal IRQ 0 timer */
    initialize_irq();           /* IRQ and/or PIC initialization */
    disable_timer_tick();       /* not needed on IBM PC as IRQ 0 vector untouched */
    save_timer_irq();           /* save original BIOS IRQ 0 vector */

    /* Connect timer interrupt handler to hardware IRQ 0 */
    if (request_irq(TIMER_IRQ, timer_tick, INT_GENERIC))
        panic("Unable to get timer");

    enable_timer_tick();        /* reprogram timer for 100 HZ */
#endif
}
