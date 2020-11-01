#ifndef __LINUXMT_KERNEL_H
#define __LINUXMT_KERNEL_H

#include <linuxmt/types.h>
#include <stddef.h>

/*
 * 'kernel.h' contains some often-used function prototypes etc
 */

#ifdef __KERNEL__

#define INT_MAX		((int)(~0U>>1))
#define UINT_MAX	(~0U)
#define LONG_MAX	((long)(~0UL>>1))
#define ULONG_MAX	(~0UL)

#define structof(p,t,m) ((t *) ((char *) (p) - offsetof (t,m)))

extern void do_exit(int);

extern int kill_pg(pid_t,sig_t,int);

extern int kill_sl(void);

extern void halt(void);
extern void panic(char *, ...);
extern void printk(char *, ...);
extern void early_putchar (int);

extern int wait_for_keypress(void);
extern int in_group_p(gid_t);

/*
 * This is defined as a macro, but at some point this might become a
 * real subroutine that sets a flag if it returns true (to do
 * BSD-style accounting where the process is flagged if it uses root
 * privs).  The implication of this is that you should do normal
 * permissions checks first, and check suser() last.
 *
 * "suser()" checks against the effective user id.
 */

extern int sys_execve(char *,char *,size_t);

#define suser() (current->euid == 0)

#endif

#endif
