/*
 * conio API for Solo86
 *
 * Ferry Hendrikx, April 2025
 */

#include <linuxmt/config.h>
#include <arch/io.h>
#include <arch/irq.h>
#include <arch/ports.h>
#include "conio.h"


/* initialize */
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
    // are there any keys waiting?

    if (inb_p(COM0_CMDS_PORT)) {
        // yes, fetch one
        int c = inb_p(COM0_DATA_PORT);

        // convert EOL
        if (c == 0x0a) c = 0x0d;

        return c;
    }
    return 0;
}

void conio_putc(byte_t c)
{
    outb(c, COM0_DATA_PORT);
}
