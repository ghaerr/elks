/* include/linuxmt/strace.h
 * (C) 1997 Chad Page
 *
 * strace allows us to track any system call going into the kernel, and the
 * return value going out.  This include file has the specs for the strace
 * tables in strace.c which will let it produce semi-readable output
 */ 

#ifndef ELKS_STRACE
#define ELKS_STRACE

/* Config parameters */

#define DEBUG_STRACE 1

/* RETWAIT tells it to to print after the system call returns.  Use this unless
 * the kernel is crashing or hanging within the system call... */
#if 0
#define STRACE_RETWAIT 1 
#endif

/* This tells strace to print stack params */

#define STRACE_PRINTSTACK 1

struct syscall_info {
	__u8 s_num;
	char *s_name;	/* Name of syscall (e.g. sys_fork or nosys_voodoo) */
	__u8 s_params;	/* # of parameters */
	__u8 t_param[3];	/* Type of parameters, defined after this */
};

struct syscall_params {
	unsigned int s_num;
	unsigned int s_param[3];
	char *s_name;
};

#define P_NONE 0
#define P_SINT 1	/* Integer */ 
#define P_UINT  2	/* Unsigned Integer */
#define P_POINTER 3 /* Regular Pointer */
#define P_PDATA 4 /* Data Pointer */
#define P_PSTR 5 /* String Pointer */ 
#define P_PLONG 6 /* Long-integer Pointer */
#define P_PULONG 7 /* __pu32 */
#define P_LONG 8
#define P_ULONG 9
/* Special parameter types go here... eventually we can process things like
 * O_RDONLY and whatnot... */

/* No special return value processing yet... sorry. */

#endif
