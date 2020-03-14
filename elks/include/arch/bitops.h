#ifndef __ARCH_8086_BITOPS_H
#define __ARCH_8086_BITOPS_H

extern unsigned char clear_bit(unsigned int,void *);
extern unsigned char set_bit(unsigned int,void *);
extern unsigned char test_bit(unsigned int,void *);

extern unsigned int find_first_non_zero_bit(register int *,unsigned int);
extern unsigned int find_first_zero_bit(register int *,unsigned int);

#endif
