#ifndef __ARCH_8086_IO_H
#define __ARCH_8086_IO_H

#define	port_MAX	0xFFFF

#ifdef __ia16__

#define outb(value,port) \
__asm__ ("outb %%al,%%dx"::"Ral" ((unsigned char)(value)),"d" (port))

#define inb(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al":"=Ral" (_v):"d" (port)); \
_v; \
})

#define outw(value,port) \
__asm__ ("outw %%ax,%%dx"::"a" ((unsigned short)(value)),"d" (port))

#define inw(port) ({ \
unsigned short _v; \
__asm__ volatile ("inw %%dx,%%ax":"=a" (_v):"d" (port)); \
_v; \
})

#define outb_p(value,port) \
__asm__ volatile ("outb %%al,%%dx\n" \
        "outb %%al,$0x80\n" \
        ::"Ral" ((unsigned char)(value)),"d" (port))

#define inb_p(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al\n" \
        "outb %%al,$0x80\n" \
        :"=Ral" (_v):"d" (port)); \
_v; \
})

#define outw_p(value,port) \
__asm__ volatile ("outw %%ax,%%dx\n" \
        "outb %%al,$0x80\n" \
        ::"a" ((unsigned short)(value)),"d" (port))

#define inw_p(port) ({ \
unsigned short _v; \
__asm__ volatile ("inw %%dx,%%ax\n" \
        "outb %%al,$0x80\n" \
        :"=a" (_v):"d" (port)); \
_v; \
})

#endif /* __ia16__ */

#endif /* !__ARCH_8086_IO_H*/
