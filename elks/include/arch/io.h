#ifndef __ARCH_8086_IO_H__
#define __ARCH_8086_IO_H__

extern void outb();
extern void outb_p();
extern void outw();
extern void outw_p();
extern unsigned char inb();
extern unsigned char inb_p();
extern unsigned short inw();
extern unsigned short inw_p();

#endif
