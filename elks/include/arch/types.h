/* arch/i86/include/asm/types.h - Basic Linux/MT data types. */

#ifndef LX86_ARCH_TYPES_H
#define LX86_ARCH_TYPES_H

/* First we define all of the __u and __s types...*/

/*@-namechecks@*/

#define signed

typedef unsigned char __u8;
typedef unsigned char *__pu8;

typedef signed char __s8;
typedef signed char *__ps8;

typedef unsigned short int __u16;
typedef unsigned short int *__pu16;

typedef signed short int __s16;
typedef signed short int *__ps16;

typedef unsigned long int __u32;
typedef unsigned long int *__pu32;

typedef signed long int __s32;
typedef signed long int *__ps32;

/* __uint == 16bit here */

typedef unsigned short int __uint;
typedef unsigned short int *__puint;

typedef signed short int __sint;
typedef signed short int *__psint;

/* Then we define registers, etc... */

struct _registers {
    __u16 ksp, sp, ss, ax, bx, cx, dx, di, si, ds, es, bp, ip, cs, flags;
};

typedef __u16 flag_t;

typedef struct _registers __registers;
typedef struct _registers *__pregisters;

typedef __u16 __pptr;		/* Changed to short as that is what it is */

struct _mminit {
    __u16 cs, endcs, ds, endds, ss, endss, lowss;
};

typedef struct _mminit __arch_mminit;
typedef struct _mminit *__parch_mminit;

/*@+namechecks@*/

#ifndef NULL
#define NULL	((void *) 0)
#endif

#endif
