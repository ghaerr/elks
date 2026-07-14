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

#include <arch/irq.h>
#include <arch/ports.h>
#include <arch/segment.h>
#include <arch/seg286.h>

/*
 * Irq index numbers >= 16 are used for hardware exceptions or syscall.
 * This allows handling of any of the 0..255 interrupts
 * including the 0..7 processor exceptions & traps.
 */
struct int_handler {
    byte_t call;        /* CALLF opcode (9Ah) */
    word_t proc;
    word_t seg;
    byte_t irq;         /* actually irq index number */
} __attribute__ ((packed));

static struct int_handler trampoline[NR_IRQS];
static irq_handler irq_action[NR_IRQS];

#ifdef CONFIG_GEM_TRAP
/*
 * Original GEM applications inspect bytes two through seven of the INT EFh
 * target and require the literal "GEMAES" signature.  The short jump skips
 * those six data bytes, then the normal ELKS far-call trampoline enters
 * _irqit.  CALLF leaves its return offset pointing at irq, exactly like the
 * compact struct int_handler used by every other ELKS interrupt.
 */
struct gem_int_handler {
    byte_t jump_opcode;          /* EBh: 8086 short relative jump */
    byte_t jump_bytes;           /* skip the six signature bytes */
    byte_t signature[6];
    byte_t call_opcode;          /* 9Ah: 8086 far call */
    word_t proc;
    word_t seg;
    byte_t irq;
} __attribute__ ((packed));

static struct gem_int_handler gem_trampoline;
static word_t gem_old_offset;
static seg_t gem_old_segment;
static byte_t gem_vector_installed;

extern void gemtrap_interrupt(int irq, struct pt_regs *regs);

/*
 * Install the broker only while a resident owner is registered.  aescheck()
 * therefore cannot mistake an ownerless kernel for a working AES service.
 */
void gemtrap_vector_enable(void)
{
    if (gem_vector_installed)
        return;

    int_vector_get(0xef, &gem_old_offset, &gem_old_segment);
    int_vector_set(0xef, (word_t)&gem_trampoline, kernel_ds);
    gem_vector_installed = 1;
}

void gemtrap_vector_disable(void)
{
    word_t offset;
    seg_t segment;

    if (!gem_vector_installed)
        return;

    /*
     * Do not overwrite a later owner that deliberately replaced INT EFh.
     * We restore the saved vector only when the IVT still points at us.
     */
    int_vector_get(0xef, &offset, &segment);
    if (offset == (word_t)&gem_trampoline && segment == kernel_ds)
        int_vector_set(0xef, gem_old_offset, gem_old_segment);
    gem_vector_installed = 0;
}

static void INITPROC gemtrap_irq_init(void)
{
    gem_trampoline.jump_opcode = 0xeb;
    gem_trampoline.jump_bytes = 6;
    gem_trampoline.signature[0] = 'G';
    gem_trampoline.signature[1] = 'E';
    gem_trampoline.signature[2] = 'M';
    gem_trampoline.signature[3] = 'A';
    gem_trampoline.signature[4] = 'E';
    gem_trampoline.signature[5] = 'S';
    gem_trampoline.call_opcode = 0x9a;
    gem_trampoline.proc = (word_t)_irqit;
    gem_trampoline.seg = KERNEL_CS;
    gem_trampoline.irq = IDX_GEM;
    irq_action[IDX_GEM] = gemtrap_interrupt;
}
#endif

/* called by _irqit assembler hook after saving registers */
void do_IRQ(int i, struct pt_regs *regs)
{
    irq_handler ih = irq_action [i];
    if (!ih)
        printk("Unexpected interrupt: %u\n", i);
    else (*ih)(i, regs);
}

/* install interrupt vector to point to handler trampoline */
static void int_handler_add(int irq, int vect, int_proc proc)
{
    struct int_handler *h;

    h = &trampoline[irq];
    h->call = 0x9A;         /* CALLF opcode */
    h->proc = (word_t)proc;
    h->seg  = KERNEL_CS;    /* resident kernel code segment */
    h->irq  = irq;
#ifdef CONFIG_286_PMODE
    /* IDT gate runs the trampoline via the data-as-code alias (trampoline[] is data) */
    idt_gate_set(vect, (word_t)h, SEL_KDATA_EXEC, GATE_INT286);
#else
    int_vector_set(vect, (word_t)h, kernel_ds);
#endif
}

/* request an IRQ from 0 to 15 */
int request_irq(int irq, irq_handler handler, int hflag)
{
    int_proc proc;
    flag_t flags;

    irq = remap_irq(irq);
    if (irq < 0 || !handler) return -EINVAL;

    if (irq_action [irq]) return -EBUSY;

    save_flags(flags);
    clr_irq();

    irq_action [irq] = handler;

    if (hflag == INT_SPECIFIC)
        proc = (int_proc) handler;
    else
        proc = _irqit;
    int_handler_add(irq, irq_vector(irq), proc);

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
    /* don't reset vector to 0:0, instead allow "unexpected interrupt" above */
    /*int_vector_set(irq_vector(irq), 0, 0);*/
    irq_action[irq] = NULL;
    restore_flags(flags);

    return 0;
}

/*
 *  IRQ setup.
 */
void INITPROC irq_init(void)
{
    int_handler_add(IDX_SYSCALL, 0x80, _irqit); /* INT 80 for system calls */

#ifdef CONFIG_GEM_TRAP
    gemtrap_irq_init();                  /* INT EF is enabled by REGISTER */
#endif

#if defined(CONFIG_ARCH_IBMPC) || defined(CONFIG_ARCH_PC98) || \
    defined(CONFIG_ARCH_SOLO86) || defined(CONFIG_ARCH_SWAN) || \
    defined(CONFIG_ARCH_NECV25)

    irq_action[IDX_DIVZERO] = div0_handler;     /* INT 0 divide by 0/divide overflow */
    int_handler_add(IDX_DIVZERO, 0x00, _irqit);

    irq_action[IDX_NMI] = nmi_handler;          /* INT 2 non-maskable interrupt */
    int_handler_add(IDX_NMI, 0x02, _irqit);
#endif

#if defined(CONFIG_ARCH_NECV25)
    irq_action[IDX_NECV25_IBRK] = ibrk_handler;     /* INT 0x13 IO Break  */
    int_handler_add(IDX_NECV25_IBRK, 0x13, _irqit);
#endif

#if defined(CONFIG_TIMER_INT0F) || defined(CONFIG_TIMER_INT1C)
    /* Use IRQ 7 vector (simulated by INT 0Fh) for timer interrupt handler */
    if (request_irq(7, timer_tick, INT_GENERIC))
        panic("Unable to get timer");

#if defined(CONFIG_TIMER_INT1C)
    /* Also map BIOS INT 1C user timer callout to INT 0Fh */
    unsigned long __far *vec1C = _MK_FP(0, 0x1C << 2);  /* BIOS user timer callout */
    unsigned long __far *vec0F = _MK_FP(0, 0x0F << 2);  /* IRQ 7 vector (INT 0Fh) */
    *vec1C = *vec0F;            /* point INT 1C to INT 0F vector */
#endif

#else /* normal IRQ 0 timer */
    initialize_irq();           /* IRQ and/or PIC initialization */
    disable_timer_tick();       /* not needed on IBM PC as IRQ 0 vector untouched */
#ifndef CONFIG_286_PMODE
    save_timer_irq();           /* save BIOS IRQ 0 vector (reads IVT via ES=0; PM uses IDT) */
#endif

    /* Connect timer interrupt handler to hardware IRQ 0 */
    if (request_irq(TIMER_IRQ, timer_tick, INT_GENERIC))
        panic("Unable to get timer");

    enable_timer_tick();        /* reprogram timer for 100 HZ */
#endif
}
