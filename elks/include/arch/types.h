/* arch/i86/include/asm/types.h - Basic Linux/MT data types. */

#ifndef __ARCH_8086_TYPES
#define __ARCH_8086_TYPES

/* First we define all of the __u and __s types...*/

typedef unsigned char __u8;
typedef unsigned char * __pu8;
typedef char __s8;
typedef char * __ps8;

typedef unsigned int __u16;
typedef unsigned int * __pu16;
typedef int __s16;
typedef int * __ps16;

typedef unsigned long __u32;
typedef unsigned long * __pu32;
typedef long __s32;
typedef long * __ps32;

/* __uint == 16bit here */

typedef unsigned int __uint;
typedef int __sint;
typedef unsigned int * __puint;
typedef int * __psint;

/* Then we define registers, etc... */

struct _registers {
	__u16 ksp, sp, ss, ax, bx, cx, dx, di, si, ds, es, bp, ip, cs, flags;
};

typedef __u16 flag_t;

typedef struct _registers __registers; 
typedef struct _registers * __pregisters;

typedef __u16 __pptr; /* Changed to short as that is what it is */

struct _mminit {
	__u16 cs, endcs, ds, endds, ss, endss, lowss;
};

typedef struct _mminit __arch_mminit;
typedef struct _mminit * __parch_mminit;

#ifndef NULL
#define NULL	0
#endif

#endif
