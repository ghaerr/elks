#ifndef LX86_ARCH_IO_H
#define LX86_ARCH_IO_H

#ifdef __BCC__
extern void outb(unsigned char, void *);
extern void outb_p(unsigned char, void *);
extern void outw(unsigned short int, void *);
extern void outw_p(unsigned short int, void *);

extern unsigned char inb(void *);
extern unsigned char inb_p(void *);

extern unsigned short int inw(void *);
extern unsigned short int inw_p(void *);

extern void bell(void);
#endif

#ifdef __WATCOMC__
extern void outb(unsigned char value, void *port);
#pragma aux outb = \
    "out dx,al"    \
    parm [al] [dx];

extern void outb_p(unsigned char value, void *port);
#pragma aux outb_p = \
    "in  al,80h"   \
    "mov al,ah"    \
    "out dx,al"    \
    parm [ah] [dx] \
    modify [al];

extern void outw(unsigned short int value, void *port);
#pragma aux outb = \
    "out dx,ax"    \
    parm [ax] [dx];

extern void outw_p(unsigned short int value, void *port);
#pragma aux outb_p = \
    "push ax"       \
    "in   al,80h"   \
    "pop  ax"       \
    "out  dx,ax"    \
    parm [ax] [dx];

extern unsigned char inb(void *port);
#pragma aux outb = \
    "in  al,dx"    \
    value [al]     \
    parm [dx];

extern unsigned char inb_p(void *port);
#pragma aux outb_p = \
    "in  al,80h"   \
    "in  al,dx"    \
    value [al]     \
    parm [dx];

extern unsigned short int inw(void *port);
#pragma aux outb = \
    "in  ax,dx"    \
    value [ax]     \
    parm [dx];

extern unsigned short int inw_p(void *port);
#pragma aux outb_p = \
    "in  al,80h"   \
    "in  ax,dx"    \
    value [ax]     \
    parm [dx];

#endif

#endif
