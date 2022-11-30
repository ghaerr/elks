/*
 *	elks/arch/i86/drivers/char/bell-8254.c
 *
 *	Copyright 1999 Greg Haerr <greg@censoft.com>
 *	Copyright 2022 TK Chia <@tkchia@mastodon.social>
 *
 *	This file rings the PC speaker.
 */

#include <linuxmt/config.h>
#include <arch/ports.h>
#include <arch/io.h>
#include <arch/irq.h>

#define BELL_FREQUENCY 800
#define BELL_PERIOD (1193181/BELL_FREQUENCY)

/*
 * Turn PC speaker on at specified frequency (more precisely, wave period).
 */
void soundp(unsigned period)
{
    clr_irq();
    outb(inb(SPEAKER_PORT) | 0x03, SPEAKER_PORT);
    outb(0xB6, TIMER_CMDS_PORT);
    outb((unsigned char)(period & 0xFF), TIMER2_PORT);
    outb((unsigned char)(period / 256), TIMER2_PORT);
    set_irq();
}

/*
 * Turn PC speaker off.
 */
void nosound(void)
{
    clr_irq();
    outb(inb(SPEAKER_PORT) & ~0x03, SPEAKER_PORT);
    set_irq();
}

/*
 * Actually sound the speaker.
 */
void bell(void)
{
    register volatile unsigned int i = 60000U;

    soundp(BELL_PERIOD);
    while (--i)
	continue;
    nosound();
}
