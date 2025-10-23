#include <arch/system.h>
#include <arch/io.h>

#define PORT_CTRL_REG 0x37
#define PORT_STATUS_SHUT 0xF0

/*
 * This function gets called by the keyboard interrupt handler.
 * As it's called within an interrupt, it may NOT sync.
 */
void ctrl_alt_del(void)
{
}

void hard_reset_now(void)
{
    outb(0x0B, PORT_CTRL_REG); /* SHUT1: 1 */
    outb(0x0F, PORT_CTRL_REG); /* SHUT0: 1 */
    if (inb(PORT_STATUS_SHUT) & 0x02) /* V30 mode */
        outb(0x07, PORT_STATUS_SHUT);
    else                              /* 286/386 mode */
        outb(0x00, PORT_STATUS_SHUT);

    asm("ljmp $0xFFFF,$0\n\t");
}

void apm_shutdown_now(void)
{
}
