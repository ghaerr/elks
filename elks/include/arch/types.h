/* arch/i86/include/asm/types.h - Basic Linux/MT data types. */

#ifndef __ARCH_8086_TYPES_H
#define __ARCH_8086_TYPES_H

/* First we define all of the __u and __s types...*/

#define signed

typedef unsigned char           __u8,       *__pu8;
typedef signed char             __s8,       *__ps8;
typedef unsigned short int      __u16,      *__pu16;
typedef signed short int        __s16,      *__ps16;
typedef unsigned long int       __u32,      *__pu32;
typedef signed long int         __s32,      *__ps32;

/* __uint == 16bit here */

typedef unsigned short int      __uint,     *__puint;
typedef signed short int        __sint,     *__psint;

/* 8086 types */
typedef __u8                    byte_t;
typedef __u16                   word_t;
typedef __u32                   long_t;
typedef __u16                   seg_t;
typedef __u16                   __pptr;
typedef __u32                   addr_t;

/* Then we define registers, etc... */

/* ordering of saved registers on kernel stack after syscall/interrupt entry*/
struct pt_regs {
    /* SI offset                 0   2        4   6   8  10  12*/
    __u16       ax, bx, cx, dx, di, si, orig_ax, es, ds, sp, ss;
};

struct xregs {
    __u16       cs;     /* code segment to use in arch_setup_user_stack()*/
    __u16       ksp;    /* saved kernel SP used by twsitch()*/
};

/* ordering of saved registers on user stack after interrupt entry*/
struct uregs {
    __u16       bp, ip, cs, f;
};

#ifndef NULL
#define NULL        ((void *) 0)
#endif

#endif /* !__ARCH_8086_TYPES_H */
