#include <linuxmt/config.h>
#include <arch/io.h>
#include <arch/irq.h>
#include <arch/swan.h>
#include "conio.h"

/* initialize*/
void conio_init(void)
{
    outb(UART_ENABLE | UART_B38400, UART_CONTROL_PORT);
}

/*
 * Poll for console input available.
 * Return nonzero character received else 0 if none ready.
 * Called approximately every ~8/100 seconds.
 */
int conio_poll(void)
{
    unsigned char c;
    unsigned char r = 0;

    c = inb(UART_CONTROL_PORT);
    if (c & UART_RX_READY)
        r = inb(UART_DATA_PORT);
    if (c & UART_OVR)
        outb((c & (UART_ENABLE | UART_B38400)) | UART_OVR_RESET,
             UART_CONTROL_PORT);
    return r;
}

void conio_putc(byte_t c)
{
    while (!(inb(UART_CONTROL_PORT) & UART_TX_READY));
    outb(c, UART_DATA_PORT);
}
