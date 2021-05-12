/*
 * 8253/8254 Programmable Interval Timer
 *
 * This file contains code used for the 8253/8254 PIT only.
 */

#include <linuxmt/config.h>
#include <linuxmt/config.h>
#include <linuxmt/mm.h>
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

#define TIMER_LO_BYTE (__u8)(((5+(11931818L/(HZ)))/10)%256)
#define TIMER_HI_BYTE (__u8)(((5+(11931818L/(HZ)))/10)/256)

void enable_timer_tick(void)
{
    /* set the clock frequency */
    outb (TIMER_MODE2, (void*)TIMER_CMDS_PORT);
    outb (TIMER_LO_BYTE, (void*)TIMER_DATA_PORT);	/* LSB */
    outb (TIMER_HI_BYTE, (void*)TIMER_DATA_PORT);	/* MSB */
}

void disable_timer_tick(void)
{
    outb (TIMER_MODE0, (void*)TIMER_CMDS_PORT);
    outb (0, (void*)TIMER_DATA_PORT);
    outb (0, (void*)TIMER_DATA_PORT);
}
