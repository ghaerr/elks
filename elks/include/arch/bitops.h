#ifndef LX86_ARCH_BITOPS_H
#define LX86_ARCH_BITOPS_H

extern unsigned char clear_bit(unsigned int,void *);
extern unsigned char set_bit(unsigned int,void *);
extern unsigned char test_bit(unsigned int,void *);

extern unsigned int find_first_non_zero_bit(void *,unsigned int);
extern unsigned int find_first_zero_bit(void *,unsigned int);

#endif
