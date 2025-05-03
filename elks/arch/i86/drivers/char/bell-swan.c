/*
 *	elks/arch/i86/drivers/char/bell-swan.c
 *
 *	This file emulates the PC speaker.
 */

#include <linuxmt/config.h>
#include <linuxmt/memory.h>
#include <arch/ports.h>
#include <arch/io.h>
#include <arch/swan.h>


#define BELL_FREQUENCY 800
#define BELL_PERIOD (1193181/BELL_FREQUENCY)

/*
 * Turn PC speaker on at specified frequency (more precisely, wave period).
 */
void soundp(unsigned period)
{
    /* FIXME: Only perform this initialization once. */
    unsigned int wave_seg = 0x60;
    int i;

    /* Write square wave pattern */
    for (i = 0; i < 8; i++) {
        pokeb(i,     (seg_t) wave_seg, 0xFF);
        pokeb(i + 8, (seg_t) wave_seg, 0x00);
    }

    /* Configure audio */
    outb(wave_seg >> 2, AUD_BASE_PORT);
    outw(0x09 << 8, AUD_CONTROL_PORT);

    /* Play sample */
    outw(period / 12, AUD_CH1_FREQ_PORT);
    outb(0xFF, AUD_CH1_VOL_PORT);
    outb(0x01, AUD_CONTROL_PORT);
}

/*
 * Turn PC speaker off.
 */
void nosound(void)
{
    outb(0x01, AUD_CONTROL_PORT);
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
