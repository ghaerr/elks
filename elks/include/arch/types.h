/* arch/i86/include/asm/types.h - Basic Linux/MT data types. */

#ifndef LX86_ARCH_TYPES_H
#define LX86_ARCH_TYPES_H

/*@-namechecks@*/

/* First we define all of the __u and __s types...*/

#define signed

typedef unsigned char			__u8,		*__pu8;
typedef signed char			__s8,		*__ps8;
typedef unsigned short int		__u16,		*__pu16;
typedef signed short int		__s16,		*__ps16;
typedef unsigned long int		__u32,		*__pu32;
typedef signed long int 		__s32,	 	*__ps32;

/* __uint == 16bit here */

typedef unsigned short int		__uint,		*__puint;
typedef signed short int		__sint,		*__psint;

/* Then we define registers, etc... */

struct _registers {
    __u16	ax, bx, cx, dx, di, si,
		es, ds, sp, ss;
};

typedef struct _registers		__registers,	*__pregisters;

struct xregs {
    __u16	cs, ksp, flags;
};

struct pt_regs {
    __u16	bp, di, si, dx, cx, bx,
		es, ds, ax, ip, cs, flags;
};

/* Changed to unsigned short int as that is what it is here.
 */

typedef __u16			__pptr;

/*@+namechecks@*/

#ifndef NULL
#define NULL		((void *) 0)
#endif

#endif
