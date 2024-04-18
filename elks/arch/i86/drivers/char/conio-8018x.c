/*
 * conio API for 8081X Headless Console
 *
 * This file contains code used for the embedded 8018X family only.
 * 
 * 16 May 21 Santiago Hormazabal
 */

#include <linuxmt/config.h>
#include <arch/io.h>
#include <arch/irq.h>
#include "conio.h"


/* initialize*/
void conio_init(void)
{
}

/*
 * Poll for console input available.
 * Return nonzero character received else 0 if none ready.
 * Called approximately every ~8/100 seconds.
 */
int conio_poll(void)
{
    /* S0STS bit 0x40 RI (receive interrupt) */
    if (inw(0xff00 + 0x66) & 0x40) {
        return inw(0xff00 + 0x68); /* R0BUF */
    }
    return 0;
}

void conio_putc(byte_t c)
{
    while((inw(0xff00 + 0x66) & 8) == 0);	/* S0STS */
    outb(c, 0xff00 + 0x6A);			/* T0BUF */
}
