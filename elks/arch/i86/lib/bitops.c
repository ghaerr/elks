#include <arch/bitops.h>
#include <arch/types.h>
#include <arch/irq.h>
#include <linuxmt/kernel.h>

/*
 *	Messy as we lack atomic bit operations on an 8086.
 */

unsigned char clear_bit(unsigned int bit, register void *addr)
{
    flag_t flags;
    unsigned int mask;

    addr = (unsigned char *)addr + (bit >> 3);
    bit &= 0x07;
    mask = 1 << bit;
    save_flags(flags);
    clr_irq();
    mask &= *(unsigned char *)addr;
    *(unsigned char *)addr &= ~mask;
    restore_flags(flags);
    return mask >> bit;
}

unsigned char set_bit(unsigned int bit, register void *addr)
{
    flag_t flags;
    unsigned int mask, r;

    addr = (unsigned char *)addr + (bit >> 3);
    bit &= 0x07;
    mask = 1 << bit;
    save_flags(flags);
    clr_irq();
    r = *(unsigned char *)addr & mask;
    *(unsigned char *)addr |= mask;
    restore_flags(flags);
    return r >> bit;
}

unsigned char test_bit(unsigned int bit,void *addr)
{
    return (((1 << (bit & 0x07)) & *((unsigned char *)addr + (bit >> 3))) != 0);
}

/* Ack... nobody even seemed to try to write to a file before 0.0.49a was
 * released, or otherwise they might have tracked it down to this being
 * non-existant :)
 * - Chad
 */

#if UNUSED
/* Use the old faithful version */
unsigned int find_first_non_zero_bit(int *addr, unsigned int len)
{
    unsigned int bit = 0;
    unsigned int mask;

    do {
	if (*addr) {
	    mask = 1;
	    while (!(*addr & mask)) {
		mask <<= 1;
		bit++;
	    }
	    break;
	}
	addr++;
    } while ((bit += 16) < len);
    if (bit > len)
	bit = len;
    return bit;
}
#endif

/* Use the old faithful version */
unsigned int find_first_zero_bit(int *addr, unsigned int len)
{
    unsigned int bit = 0;
    unsigned int mask;

    do {
	if (~(*addr)) {
	    mask = 1;
	    while (*addr & mask) {
		mask <<= 1;
		bit++;
	    }
	    break;
	}
	addr++;
    } while ((bit += 16) < len);
    if (bit > len)
	bit = len;
    return bit;
}
