#ifndef __ARCH_8086_BITOPS_H
#define __ARCH_8086_BITOPS_H

unsigned char clear_bit(unsigned int,void *);
unsigned char set_bit(unsigned int,void *);
unsigned char test_bit(unsigned int,void *);

unsigned int find_first_non_zero_bit(int *,unsigned int);
unsigned int find_first_zero_bit(int *,unsigned int);

#endif
