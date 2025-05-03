#include <arch/system.h>

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
    asm("mov $0x40,%ax\n\t"
        "mov %ax,%ds\n\t"
        "movw $0x1234,0x72\n\t"
        "ljmp $0xFFFF,$0\n\t"
    );
}

/*
 *  Use Advanced Power Management to power off system
 *  For details on how this code works, see
 *  http://wiki.osdev.org/APM
 */
void apm_shutdown_now(void)
{
    asm("movw $0x5301,%ax\n\t"
        "xorw %bx,%bx\n\t"
        "int $0x15\n\t"
        "jc apm_error\n\t"
        "movw $0x5308,%ax\n\t"
        "movw $1,%bx\n\t"
        "movw $1,%cx\n\t"
        "int $0x15\n\t"
        "jc apm_error\n\t"
        "movw $0x5307,%ax\n\t"
        "movw $1,%bx\n\t"
        "movw $3,%cx\n\t"
        "int $0x15\n\t"
        "apm_error:\n\t"
    );
}
