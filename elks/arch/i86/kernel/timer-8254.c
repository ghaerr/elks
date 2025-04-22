/*
 * 8253/8254 Programmable Interval Timer
 *
 * This file contains code used for the 8253/8254 PIT only.
 */

#include <linuxmt/config.h>
#include <linuxmt/config.h>
#include <linuxmt/mm.h>
#include <linuxmt/memory.h>
#include <linuxmt/timer.h>

#include <arch/io.h>
#include <arch/irq.h>
#include <arch/param.h>
#include <arch/ports.h>

/*
 * 4/2001 Use macros to directly generate timer constants based on the
 *        value of HZ macro. This works the same for both the 8253 and
 *        8254 timers. - Thomas McWilliams  <tmwo@users.sourceforge.net>
 *
 *  These 8253/8254 macros generate proper timer constants based on the
 *  timer tick macro HZ which is defined in param.h (usually 100 Hz).
 *
 *  The PC timer chip can be programmed to divide its reference frequency
 *  by a 16 bit unsigned number. The reference frequency of 1193181.8 Hz
 *  happens to be 1/3 of the NTSC color burst frequency. In fact, the
 *  hypothetical exact reference frequency for the timer is 39375000/33 Hz.
 *  The macros use scaled fixed point arithmetic for greater accuracy.
 */

#define TIMER_MODE0 0x30   /* timer 0, binary count, mode 0, lsb/msb */
#define TIMER_MODE2 0x34   /* timer 0, binary count, mode 2, lsb/msb */

#ifdef CONFIG_ARCH_IBMPC
#define TIMER_LO_BYTE (__u8)(((5+(11931818L/(HZ)))/10)%256)
#define TIMER_HI_BYTE (__u8)(((5+(11931818L/(HZ)))/10)/256)
#endif

#ifdef CONFIG_ARCH_PC98
#define TIMER_LO_BYTE_5M (__u8)(((5+(24576000L/(HZ)))/10)%256)
#define TIMER_HI_BYTE_5M (__u8)(((5+(24576000L/(HZ)))/10)/256)
#define TIMER_LO_BYTE_8M (__u8)(((5+(19968000L/(HZ)))/10)%256)
#define TIMER_HI_BYTE_8M (__u8)(((5+(19968000L/(HZ)))/10)/256)
#endif

#ifdef CONFIG_ARCH_SOLO86
#define TIMER_LO_BYTE (__u8)(((5+(10000000L/(HZ)))/10)%256)
#define TIMER_HI_BYTE (__u8)(((5+(10000000L/(HZ)))/10)/256)
#define TIMER_ENABLE  1
#endif

void enable_timer_tick(void)
{
    /* set the clock frequency */
    outb (TIMER_MODE2, TIMER_CMDS_PORT);

#if defined(CONFIG_ARCH_IBMPC) || defined(CONFIG_ARCH_SOLO86)
    outb (TIMER_LO_BYTE, TIMER_DATA_PORT);      /* LSB */
    outb (TIMER_HI_BYTE, TIMER_DATA_PORT);      /* MSB */
#endif

#ifdef CONFIG_ARCH_SOLO86
    outb(TIMER_ENABLE, TIMER_ENBL_PORT);        /* enable TIMER */
#endif

#ifdef CONFIG_ARCH_PC98
    if (peekb(0x501, 0) & 0x80) {
        printk("Timer clock frequncy for 8MHz system is set.\n");
        outb (TIMER_LO_BYTE_8M, TIMER_DATA_PORT);   /* LSB */
        outb (TIMER_HI_BYTE_8M, TIMER_DATA_PORT);   /* MSB */
    } else {
        printk("Timer clock frequncy for 5MHz system is set.\n");
        outb (TIMER_LO_BYTE_5M, TIMER_DATA_PORT);   /* LSB */
        outb (TIMER_HI_BYTE_5M, TIMER_DATA_PORT);   /* MSB */
    }
#endif
}

void disable_timer_tick(void)
{
#if NOTNEEDED   /* not needed on IBM PC as IRQ 0 vector untouched */
    outb (TIMER_MODE0, TIMER_CMDS_PORT);
    outb (0, TIMER_DATA_PORT);
    outb (0, TIMER_DATA_PORT);
#endif
}
