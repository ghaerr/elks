#ifndef LX86_LINUXMT_STRACE_H
#define LX86_LINUXMT_STRACE_H

/* include/linuxmt/strace.h (C) 1997 Chad Page
 *
 * strace allows us to track any system call going into the kernel, and the
 * return value going out.  This include file has the specs for the strace
 * tables in strace.c which will let it produce semi-readable output
 */

/* Config parameters */

/* This tells strace to print stack params */

#define STRACE_PRINTSTACK 1

struct syscall_info
{
    __u8 s_num;
    char *s_name;		/* Name of syscall (e.g. sys_fork or nosys_voodoo) */
    __u8 s_params;		/* # of parameters */
    __u8 t_param[3];		/* Type of parameters, defined after this */
};

struct syscall_params
{
    unsigned int s_num;
    unsigned int s_param[3];
    char *s_name;
};

/* Values reorganised as follows:
 *
 * Bits  Values  Meaning
 * ~~~~  ~~~~~~  ~~~~~~~
 *  1-0    00    Unsigned
 *         01    Signed
 *         10    Pointer to unsigned
 *         11    Pointer to signed
 *  2+           Data Type
 */

#define P_NONE		  0	/* No parameters                        */
#define P_DATA		  1	/* Generic Data                         */
#define P_POINTER	  2	/* Generic Data Pointer                 */
#define P_PDATA 	  3	/* Pointer to Generic Data              */

#define P_UCHAR 	  4	/* Unsigned Char                        */
#define P_SCHAR 	  5	/* Signed Char                          */
#define P_STR		  6	/* String                               */
#define P_PSTR  	  7	/* Pointer to String                    */

#define P_USHORT	  8	/* Unsigned Short Int                   */
#define P_SSHORT	  9	/* Signed Short Int                     */
#define P_PUSHORT 	 10	/* Pointer to Unsigned Short Int        */
#define P_PSSHORT 	 11	/* Pointer to Signed Short Int          */

#define P_ULONG 	 12	/* Unsigned Long Int                    */
#define P_SLONG 	 13	/* Signed Long Int                      */
#define P_PULONG 	 14	/* Pointer to Unsigned Long Int         */
#define P_PSLONG	 15	/* Pointer to Signed Long Int           */

#ifdef NOT_YET

#define P_ULLONG	 16	/* Unsigned Long Long Int               */
#define P_SLLONG	 17	/* Signed Long Long Int                 */
#define P_PULLONG 	 18	/* Pointer to Unsigned Long Long Int    */
#define P_PSLLONG	 19	/* Pointer to Signed Long Long Int      */

#endif

/* Special parameter types go here... eventually we can process things
 * like O_RDONLY and whatnot... */

/* No special return value processing yet... sorry. */

#endif
