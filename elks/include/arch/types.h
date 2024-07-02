#ifndef __ARCH_8086_TYPES_H
#define __ARCH_8086_TYPES_H

/* Basic ELKS data types. */

/* First we define all of the __u and __s types...*/

typedef unsigned char           __u8;
typedef signed char             __s8;
typedef unsigned short int      __u16;
typedef signed short int        __s16;
typedef unsigned long int       __u32;
typedef signed long int         __s32;

/* 8086 types */
typedef __u8                    byte_t;
typedef __u16                   word_t;
typedef __u32                   long_t;

typedef __u16                   seg_t;      /* segment value */
typedef __u16                   segext_t;   /* paragraph count */
typedef __u16                   segoff_t;   /* offset in segment */
typedef __u16                   flag_t;     /* CPU flag word */

typedef __u32                   addr_t;     /* linear 32-bit address */

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

/*
 * We define various special typedefs here rather than
 * including the compiler vendor's headers, which may
 * drag in other conflicting definitions.
 */

/* <stddef.h> */
typedef unsigned    size_t;
#define offsetof(__typ,__id) ((size_t)((char *)&(((__typ*)0)->__id) - (char *)0))

/* <sys/types.h> */
typedef signed int  ssize_t;
typedef long long   int64_t;

/* <stdint.h> */
#ifdef __GNUC__
typedef int             intptr_t;   /* ia16-elf-gcc supports small/medium models only */
typedef unsigned int    uintptr_t;
#endif

#ifdef __WATCOMC__
#if defined(__COMPACT__) || defined(__LARGE__)
typedef long            intptr_t;
typedef unsigned long   uintptr_t;
#else
typedef int             intptr_t;
typedef unsigned int    uintptr_t;
#endif
#endif

#endif
