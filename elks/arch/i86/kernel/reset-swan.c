#include <arch/system.h>
#include <arch/io.h>
#include <arch/irq.h>
#include <arch/swan.h>

/*
 * This function gets called by the keyboard interrupt handler.
 * As it's called within an interrupt, it may NOT sync.
 */
void ctrl_alt_del(void)
{
    hard_reset_now();
}

void hard_reset_now(void)
{
    /* Disable interrupts */
    outb(0x00, 0xB2);
    outb(0xFF, 0xB6);
    clr_irq();
    /* Hard reset */
    asm("ljmp $0xFFFF,$0\n\t");
}

void apm_shutdown_now(void)
{
    /* Disable interrupts */
    outb(0x00, 0xB2);
    outb(0xFF, 0xB6);
    clr_irq();
    /* Request poweroff */
    outb(1, 0x62);
    /* Halt CPU */
    while(1) asm("hlt");
}
