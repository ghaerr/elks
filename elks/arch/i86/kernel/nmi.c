#include <linuxmt/config.h>
#include <arch/io.h>

/*
 * NMI handler
 */

static char nmi_msg[] = { "NMI occurred\n" };

void nmi_handler(int i, struct pt_regs *regs)
{
    printk(nmi_msg);

#ifdef CONFIG_ARCH_PC98
    if (inb(0x33) & 0x02)
        printk("Extended slot memory parity error\n");

    outb(0x08,0x37); /* Disable parity check to clear the error */
    outb(0x09,0x37); /* Enable parity check */
#endif
}
