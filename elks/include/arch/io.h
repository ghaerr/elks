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

#define insb(port,seg,offset,cnt)   \
    __extension__ ({                \
        unsigned ax, cx, di;        \
        asm volatile (              \
            "push %%es\n"           \
            "mov %%ax,%%es\n"       \
            "cld\n"                 \
            "1:\n"                  \
            "in (%%dx),%%al\n"      \
            "stosb\n"               \
            "loop 1b\n"             \
            "pop %%es\n"            \
            : "=a" (ax), "=c" (cx), "=D" (di)                   \
            : "d" (port), "a" (seg), "D" (offset), "c" (cnt)    \
            : "memory" );           \
    })

#define insw(port,seg,offset,cnt)   \
    __extension__ ({                \
        unsigned ax, cx, di;        \
        asm volatile (              \
            "push %%es\n"           \
            "mov %%ax,%%es\n"       \
            "cld\n"                 \
            "1:\n"                  \
            "in (%%dx),%%ax\n"      \
            "stosw\n"               \
            "loop 1b\n"             \
            "pop %%es\n"            \
            : "=a" (ax), "=c" (cx), "=D" (di)                   \
            : "d" (port), "a" (seg), "D" (offset), "c" (cnt)    \
            : "memory" );           \
    })


#define outsb(port,seg,offset,cnt)  \
    __extension__ ({                \
        unsigned ax, cx, si;        \
        asm volatile (              \
            "push %%ds\n"           \
            "mov %%ax,%%ds\n"       \
            "cld\n"                 \
            "1:\n"                  \
            "lodsb\n"               \
            "out %%al,(%%dx)\n"     \
            "loop 1b\n"             \
            "pop %%ds\n"            \
            : "=a" (ax), "=c" (cx), "=S" (si)                   \
            : "d" (port), "a" (seg), "S" (offset), "c" (cnt)    \
            : "memory" );           \
    })

#define outsw(port,seg,offset,cnt)  \
    __extension__ ({                \
        unsigned ax, cx, si;        \
        asm volatile (              \
            "push %%ds\n"           \
            "mov %%ax,%%ds\n"       \
            "cld\n"                 \
            "1:\n"                  \
            "lodsw\n"               \
            "out %%ax,(%%dx)\n"     \
            "loop 1b\n"             \
            "pop %%ds\n"            \
            : "=a" (ax), "=c" (cx), "=S" (si)                   \
            : "d" (port), "a" (seg), "S" (offset), "c" (cnt)    \
            : "memory" );           \
    })

#endif /* __ia16__ */

#endif /* !__ARCH_8086_IO_H*/
