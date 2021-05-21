/*
 *	elks/arch/i86/drivers/char/bell.c
 *
 *	Copyright 1999 Greg Haerr <greg@censoft.com>
 *
 *	This file rings the PC speaker at a specified frequency.
 */

#include <arch/ports.h>
#include <arch/io.h>

#define BELL_FREQUENCY 800
#define BELL_PERIOD (1193181/BELL_FREQUENCY)
#define BELL_PERIOD_L (unsigned char)(BELL_PERIOD & 0xFF)
#define BELL_PERIOD_H (unsigned char)(BELL_PERIOD / 256)

/*
 * Turn PC speaker on at specified frequency.
 */
static void sound(void)
{
    outb(inb(SPEAKER_PORT) | 0x03, SPEAKER_PORT);
    outb(0xB6, TIMER_CMDS_PORT);
    outb(BELL_PERIOD_L, TIMER2_PORT);
    outb(BELL_PERIOD_H, TIMER2_PORT);
}

/*
 * Turn PC speaker off.
 */
static void nosound(void)
{
    outb(inb(SPEAKER_PORT) & ~0x03, SPEAKER_PORT);
}

/*
 * Actually sound the speaker.
 */
void bell(void)
{
    register unsigned int i = 60000U;

    sound();
    while (--i)
	continue;
    nosound();
}
