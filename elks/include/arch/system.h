#ifndef __ARCH_8086_SYSTEM_H
#define __ARCH_8086_SYSTEM_H

/* sys_reboot flag parameter */
#define RB_REBOOT      0x0123           /* hard reset */
#define RB_SHUTDOWN    0x6789           /* halt system */
#define RB_POWEROFF    0xDEAD           /* call BIOS APM */

#ifdef __KERNEL__
#include <linuxmt/types.h>
#include <linuxmt/init.h>

extern byte_t sys_caps;     /* system capabilities bits*/
extern seg_t membase;       /* start and end segment of available main memory */
extern seg_t memend;

unsigned int INITPROC setup_arch(void);
void ctrl_alt_del(void);
void hard_reset_now(void);
void apm_shutdown_now(void);
#endif

#endif
