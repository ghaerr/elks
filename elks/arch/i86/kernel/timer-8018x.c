/*
 * 8018X's Integrated Timer/Counter Unit
 *
 * This file contains code used for the embedded 8018X family only.
 * 
 * 16 May 21 Santiago Hormazabal
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
 * Main goal is to have Timer2 generating a 1ms period signal, so
 * calculate the appropriate value of the prescaler counter.
 * Note that 1kHz corresponds to 1ms period. Timer2 is clocked
 * by 1/4 of FCPU.
 *
 * Timer2Interval = (FCPU / 4) / 1kHz
 */
#define TIMER2_INTERVAL (CONFIG_8018X_FCPU * 1000000L / (4 * 1000))

/*
 * With a reference of 1ms (1kHz) coming into Timer1, calculate how
 * many counts are needed to achieve the "HZ" rate (usually 100Hz).
 *
 * Timer1Interval = 1kHz / HZ
 */
#define TIMER1_INTERVAL (1000 / HZ)

void enable_timer_tick(void)
{
    /* Set Timer2 Maxcount register */
    outw(TIMER2_INTERVAL, 0xff00 + 0x42); /* T2CMPA */
    /* Clear the Timer count register before starting it */
    outw(0x0000, 0xff00 + 0x40); /* T2CNT */

    /* Set Timer1 Maxcount register */
    outw(TIMER1_INTERVAL, 0xff00 + 0x3a); /* T1CMPA */
    /* Clear the Timer count register before starting it */
    outw(0x0000, 0xff00 + 0x38); /* T1CNT */

    /* Enable Timer2, release inhibit to change EN bit, continuous mode */
    outw(0xc001, 0xff00 + 0x46); /* T2CON */

    /* Enable Timer1, release inhibit to change EN bit, INT enabled, use Timer2 as source, continuous mode */
    outw(0xe009, 0xff00 + 0x3e); /* T1CON */
}

void disable_timer_tick(void)
{
    /* Disable Timer2, release inhibit to change EN bit */
    outw(0x4000, 0xff00 + 0x46); /* T2CON */

    /* Disable Timer1, release inhibit to change EN bit */
    outw(0x4000, 0xff00 + 0x3e); /* T1CON */
}
