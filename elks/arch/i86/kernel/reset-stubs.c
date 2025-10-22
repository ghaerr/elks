#include <arch/system.h>

/*
 * This function gets called by the keyboard interrupt handler.
 * As it's called within an interrupt, it may NOT sync.
 */
void ctrl_alt_del(void)
{
}

void hard_reset_now(void)
{
#if defined CONFIG_ARCH_PC98
    asm("mov $0x37,%dx\n\t"
        "mov $0xb,%al\n\t"
        "out %al,%dx\n\t"
        "mov $0xf,%al\n\t"
        "out %al,%dx\n\t"
        "mov $0xf0,%dx\n\t"
        "in  %dx,%al\n\t"
        "and $2,%al\n\t"
        "cmp $2,%al\n\t"
        "je V30\n\t"
        "mov $0,%al\n\t"
        "out %al,%dx\n\t"
        "ljmp $0xFFFF,$0\n\t"
"V30:\n\t"
        "mov $7,%al\n\t"
        "out %al,%dx\n\t"
        "ljmp $0xFFFF,$0\n\t"
    );

#endif
}

void apm_shutdown_now(void)
{
}
