/*
 *	elks/arch/i86/drivers/char/bell-yes.c
 *
 *	Beeper for the Philips :YES.
 *
 *	The ROM provides it through its video/system call, INT 5Dh:
 *
 *	    AX=1020h  CX=pitch	set the beep pitch
 *	    AX=1021h  beep
 *
 */

#include <linuxmt/config.h>
#include <linuxmt/kd.h>

/* CX for bell(). 10 is shrill, 1000 is about a normal terminal beep. */
#define BELL_PITCH	1000

/* The ROM beep spins with interrupts enabled and returns that way, so this
 * must not be called from a context that needs them off. */
static void yes_beep(unsigned pitch)
{
    asm volatile ("int $0x5d" : : "a" (0x1020), "c" (pitch)
                  : "bx", "dx", "si", "di", "memory");
    asm volatile ("int $0x5d" : : "a" (0x1021)
                  : "bx", "cx", "dx", "si", "di", "memory");
}

void bell(void)
{
    yes_beep(BELL_PITCH);
}

/*
 * KIOCSOUND. The ROM has no way to start a tone and leave it running: its
 * beep blocks for a fixed duration and stops on its own. So a request for a
 * tone produces one beep at the nearest pitch, and the matching "stop" has
 * nothing left to do.
 */
void soundp(unsigned period)
{
    yes_beep(period ? period : BELL_PITCH);
}

void nosound(void)
{
}
