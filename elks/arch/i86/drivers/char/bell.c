/*
 *	elks/arch/i86/drivers/char/bell.c
 *
 *	Copyright 1999 Greg Haerr <greg@censoft.com>
 *
 *	This file rings the PC speaker at a specified frequency.
 */

#include <arch/io.h>

#define BELL_FREQUENCY 800
#define BELL_PERIOD (1193181/BELL_FREQUENCY)
#define BELL_PERIOD_L (BELL_PERIOD & 0xFF)
#define BELL_PERIOD_H (BELL_PERIOD / 256)
#define SPEAKER_PORT (0x61)
#define TIMER2_PORT (0x42)
#define TIMER_CONTROL_PORT (0x43)

/*
 * Turn PC speaker on at specified frequency.
 */
static void sound(void)
{
    asm(\
	"\tin	al,0x61\n" \
	"\tor	al,#3\n" \
	"\tout	0x61,al\n" \
	"\tmov	al,#0xB6\n" \
	"\tout	0x43,al\n" \
	"\tmov	al,#0xD3\n" \
	"\tout	0x42,al\n" \
	"\tmov	al,#0x05\n" \
	"\tout	0x42,al\n" \
	);
}

/*
 * Turn PC speaker off.
 */
static void nosound(void)
{
    asm(\
	"\tin	al,0x61\n" \
	"\tand	al,#0xFC\n" \
	"\tout	0x61,al\n" \
	);
}

/*
 * Actually sound the speaker.
 */
void bell(void)
{
    register char *pi = (char *) 60000U;

    sound();
    while (--pi)
	/* Do nothing */ ;
    nosound();
}
