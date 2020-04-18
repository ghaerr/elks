#include <linuxmt/config.h>
#include <linuxmt/mm.h>
#include <linuxmt/timer.h>
#include <linuxmt/timex.h>

#include <arch/io.h>
#include <arch/irq.h>
#include <arch/ports.h>

/*
 *	Timer tick routine
 *
 * 9/1999 The 100 Hz system timer 0 can configure for variable input
 *        frequency. Christian Mardm"oller (chm@kdt.de)
 *
 * 4/2001 Use macros to directly generate timer constants based on the
 *        value of HZ macro. This works the same for both the 8253 and
 *        8254 timers. - Thomas McWilliams  <tmwo@users.sourceforge.net>
 */

jiff_t jiffies = 0;

extern void do_timer(struct pt_regs *);
extern void keyboard_irq(int, struct pt_regs *, void *);

#ifndef CONFIG_ARCH_SIBO

/*  These 8253/8254 macros generate proper timer constants based on the
 *  timer tick macro HZ which is defined in timex.h (usually 100 Hz).
 *
 *  The PC timer chip can be programmed to divide its reference frequency
 *  by a 16 bit unsigned number. The reference frequency of 1193181.8 Hz
 *  happens to be 1/3 of the NTSC color burst frequency. In fact, the
 *  hypothetical exact reference frequency for the timer is 39375000/33 Hz.
 *  The macros use scaled fixed point arithmetic for greater accuracy.
 */

#define TIMER_MODE0 0x30   /* timer 0, binary count, mode 0, lsb/msb */
#define TIMER_MODE2 0x34   /* timer 0, binary count, mode 2, lsb/msb */

#define TIMER_LO_BYTE (__u8)(((5+(11931818L/(HZ)))/10)%256)
#define TIMER_HI_BYTE (__u8)(((5+(11931818L/(HZ)))/10)/256)

#endif

void timer_tick(int irq, struct pt_regs *regs, void *data)
{
    do_timer(regs);

#ifndef CONFIG_ARCH_SIBO

#if 0
    pokew(40, 0xb80a, peekw(40, 0xb80a)+1);
#endif

#ifdef CONFIG_DEBUG_TIMER
    outb (jiffies, 0x80);
#endif

#else

    /* As we are now responsible for clearing interrupt */
    clr_irq();

    outb(0x0, 0x0A);
    outb(0x0, 0x0C);
    outb(0x0, 0x10);

    set_irq();

    keyboard_irq(1, regs, NULL);

#endif
}

void enable_timer_tick(void)
{
#ifndef CONFIG_ARCH_SIBO
    /* set the clock frequency */
    outb (TIMER_MODE2, (void*)TIMER_CMDS_PORT);
    outb (TIMER_LO_BYTE, (void*)TIMER_DATA_PORT);	/* LSB */
    outb (TIMER_HI_BYTE, (void*)TIMER_DATA_PORT);	/* MSB */

#else

    outb (0x00, 0x15);
    outb (0x02, 0x08);
    printk("Timer enabled...\n");

#endif
}

void stop_timer(void)
{
#ifndef CONFIG_ARCH_SIBO
    outb (TIMER_MODE0, (void*)TIMER_CMDS_PORT);
    outb (0, (void*)TIMER_DATA_PORT);
    outb (0, (void*)TIMER_DATA_PORT);
#endif
}
