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
#include <arch/ports.h>

#ifdef CONFIG_ARCH_PC98
#undef inb_p
#define inb_p   inb             /* no delay using I/O port 0x80 */
#endif

/*
 *  Low level interrupt handling for the X86 PC/XT and PC/AT platform
 */

void initialize_irq(void)
{
#ifdef CONFIG_286_PMODE
    /* In protected mode the BIOS-programmed PIC (IRQ0-7 -> INT 0x08-0x0F) collides
     * with the CPU exception vectors.  Reprogram the 8259s: IRQ0-15 -> INT 0x20-0x2F. */
    outb(0x11, PIC1_CMD);       /* ICW1: cascade, edge-triggered, ICW4 to follow */
    outb(0x11, PIC2_CMD);
    outb(0x20, PIC1_DATA);      /* ICW2: master vector base = 0x20 */
    outb(0x28, PIC2_DATA);      /* ICW2: slave  vector base = 0x28 */
    outb(0x04, PIC1_DATA);      /* ICW3: slave is attached to master IRQ2 */
    outb(0x02, PIC2_DATA);      /* ICW3: slave cascade identity = 2 */
    outb(0x01, PIC1_DATA);      /* ICW4: 8086/88 mode */
    outb(0x01, PIC2_DATA);
    outb(0xFB, PIC1_DATA);      /* mask all master IRQs except IRQ2 (cascade) */
    outb(0xFF, PIC2_DATA);      /* mask all slave IRQs; enable_irq() unmasks on request */
#endif
#if NOTNEEDED   /* not needed on IBM PC as BIOS initializes IRQ 2 */
    if (sys_caps & CAP_IRQ2MAP9) {      /* PC/AT or greater */
        save_flags(flags);
        clr_irq();
        enable_irq(2);          /* Cascade slave PIC */
        restore_flags(flags);
    }
#endif
}

void enable_irq(unsigned int irq)
{
    unsigned char mask;

    mask = ~(1 << (irq & 7));
    if (irq < 8) {
        unsigned char cache_21 = inb_p(PIC1_DATA);
        cache_21 &= mask;
        outb(cache_21, PIC1_DATA);
    } else {
        unsigned char cache_A1 = inb_p(PIC2_DATA);
        cache_A1 &= mask;
        outb(cache_A1, PIC2_DATA);
    }
}

int remap_irq(int irq)
{
    if ((unsigned int)irq > 15 || (irq > 7 && !(sys_caps & CAP_IRQ8TO15)))
        return -EINVAL;
    if (irq == 2 && (sys_caps & CAP_IRQ2MAP9))
        irq = 9;                /* Map IRQ 9/2 over */
    return irq;
}

// Get interrupt vector from IRQ

int irq_vector (int irq)
{
#ifdef CONFIG_286_PMODE
        // PM: PIC remapped above the CPU exceptions -> IRQ 0-15 = INT 0x20-0x2F
        return irq + 0x20;
#elif defined(CONFIG_ARCH_PC98)
        // IRQ 0-7  are mapped to vectors INT 08h-0Fh
        // IRQ 8-15 are mapped to vectors INT 10h-17h

        return irq + 0x08;
#else
        // IRQ 0-7  are mapped to vectors INT 08h-0Fh
        // IRQ 8-15 are mapped to vectors INT 70h-77h

        return irq + ((irq >= 8) ? 0x68 : 0x08);
#endif
}

void disable_irq(unsigned int irq)
{
    flag_t flags;
    unsigned char mask = 1 << (irq & 7);

    save_flags(flags);
    clr_irq();
    if (irq < 8) {
        unsigned char cache_21 = inb_p(PIC1_DATA);
        cache_21 |= mask;
        outb(cache_21, PIC1_DATA);
    } else {
        unsigned char cache_A1 = inb_p(PIC2_DATA);
        cache_A1 |= mask;
        outb(cache_A1, PIC2_DATA);
    }
    restore_flags(flags);
}
