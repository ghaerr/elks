#include <arch/bitops.h>
#include <arch/types.h>
#include <arch/irq.h>
#include <linuxmt/kernel.h>

/*
 *	Messy as we lack atomic bit operations on an 8086.
 */
 
unsigned char clear_bit(unsigned int bit,void *addr)
{
    register unsigned char *ptr;
    flag_t flags;
    unsigned int mask;

	ptr = ((unsigned char *)addr) + (bit / 8);
    bit %= 8;
    mask = (1 << bit);
    save_flags(flags);
    clr_irq();
	mask &= *ptr;
    *ptr &= ~mask;
    restore_flags(flags);
    return mask >> bit;
}

unsigned char set_bit(unsigned int bit,void *addr)
{
    register unsigned char *ptr;
    flag_t flags;
    unsigned int mask, r;

	ptr = ((unsigned char *)addr) + (bit / 8);
    bit %= 8;
    mask = (1 << bit);
    save_flags(flags);
    clr_irq();
	r = *ptr & mask;
    *ptr |= mask;
    restore_flags(flags);
    return r >> bit;
}

unsigned char test_bit(unsigned int bit,void *addr)
{
	return ( ((1 << (bit % 8))
			  & ((unsigned char *) addr)[bit / 8]) != 0);
}

/* Ack... nobody even seemed to try to write to a file before 0.0.49a was
 * released, or otherwise they might have tracked it down to this being
 * non-existant :) 
 * - Chad
 */

/* Use the old faithful version */
unsigned int find_first_non_zero_bit(void *addr, unsigned int len)
{
	register char *pi = 0;

	while (((unsigned int) pi) < len) {
		if (test_bit(((unsigned int) pi), addr)) {
			break;
		}
		++pi;
	}
	return (unsigned int) pi;
}

/* Use the old faithful version */
unsigned int find_first_zero_bit(void *addr, unsigned int len)
{
	register char *pi = 0;

	while (((unsigned int) pi) < len) {
		if (!test_bit(((unsigned int) pi), addr)) {
			break;
		}
		++pi;
	}
	return (unsigned int) pi;
}
