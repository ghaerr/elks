#ifndef LX86_ARCH_IO_H
#define LX86_ARCH_IO_H

extern void outb(unsigned char, void *);
extern void outb_p(unsigned char, void *);
extern void outw(unsigned short int, void *);
extern void outw_p(unsigned short int, void *);

extern unsigned char inb(void *);
extern unsigned char inb_p(void *);
extern unsigned short int inw(void *);
extern unsigned short int inw_p(void *);

#endif
