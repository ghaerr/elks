#ifndef __ARCH_8086_IO_H
#define __ARCH_8086_IO_H

#define	port_MAX	0xFFFF

#ifdef __ia16__

#define outb(value,port)                \
        asm volatile ("outb %%al,%%dx"  \
            : /* no output */           \
            :"Ral" ((unsigned char)(value)),"d" (port))

#define inb(port) __extension__ ({      \
        unsigned char _v;               \
        asm volatile ("inb %%dx,%%al"   \
            :"=Ral" (_v)                \
            :"d" (port));               \
        _v; })

#define outw(value,port)                \
        asm volatile ("outw %%ax,%%dx"  \
            : /* no output */           \
            :"a" ((unsigned short)(value)),"d" (port))

#define inw(port) __extension__ ({      \
        unsigned short _v;              \
        asm volatile ("inw %%dx,%%ax"   \
            :"=a" (_v)                  \
            :"d" (port));               \
        _v; })

#define outb_p(value,port)                \
        asm volatile ("outb %%al,%%dx\n"  \
                      "outb %%al,$0x80\n" \
                        : /* no output */ \
                        :"Ral" ((unsigned char)(value)),"d" (port))

#define inb_p(port) __extension__ ({      \
        unsigned char _v;                 \
        asm volatile ("inb %%dx,%%al\n"   \
                      "outb %%al,$0x80\n" \
                        :"=Ral" (_v)      \
                        :"d" (port));     \
        _v; })

#define outw_p(value,port)                \
        asm volatile ("outw %%ax,%%dx\n"  \
                      "outb %%al,$0x80\n" \
                        : /* no output */ \
                        :"a" ((unsigned short)(value)),"d" (port))

#define inw_p(port) __extension__ ({      \
        unsigned short _v;                \
        asm volatile ("inw %%dx,%%ax\n"   \
                      "outb %%al,$0x80\n" \
                        :"=a" (_v)        \
                        :"d" (port));     \
        _v; })

#endif /* __ia16__ */

#endif /* !__ARCH_8086_IO_H*/
