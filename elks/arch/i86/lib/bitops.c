#include <arch/bitops.h>
#include <arch/types.h>
#include <arch/irq.h>
#include <linuxmt/kernel.h>

/*
 *	Messy as we lack atomic bit operations on an 8086.
 */
 
unsigned char clear_bit(unsigned short int bit,void *addr)
{
    unsigned char *ptr = addr;
    unsigned int r, mask, offset = (bit / 8);
    flag_t flags;

    bit%=8;
    mask = (1 << bit);
    save_flags(flags);
    i_cli();
    r = ptr[offset] & mask;
    ptr[offset] &= ~mask;	/* xor bit with itself is 0 */
    restore_flags(flags);
    return (r ? 1:0);
}

unsigned char set_bit(unsigned short int bit,void *addr)
{
    unsigned char *ptr = addr;
    unsigned int r, mask, offset = (bit / 8);
    flag_t flags;

    bit %= 8;
    mask = (1 << bit);
    save_flags(flags);
    i_cli();
    r = ptr[offset] & mask;
    ptr[offset] |= mask;	/* xor bit with itself is 0 */
    restore_flags(flags);
    return (r ? 1 : 0);
}

unsigned char test_bit(unsigned short int bit,void *addr)
{
    unsigned char *ptr = addr;
    unsigned int mask, offset = bit / 8;

    bit %= 8;
    mask = 1 << bit;
    return ((mask & ptr[offset]) != 0);
}

/* Ack... nobody even seemed to try to write to a file before 0.0.49a was
 * released, or otherwise they might have tracked it down to this being
 * non-existant :) 
 * - Chad
 */

/* Use the old faithful version */
unsigned short int find_first_non_zero_bit(void *addr, unsigned short int len)
{
    unsigned short int i;

    for (i = 0; i < len; i++)
	if (test_bit(i, addr))
	    break;
    return i;
}

/* Use the old faithful version */
unsigned short int find_first_zero_bit(void *addr,unsigned short int len)
{
    unsigned short int i;

    for (i = 0; i < len; i++)
	if (!test_bit(i, addr))
	    break;
    return i;
}
