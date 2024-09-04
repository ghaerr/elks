#ifndef __ARCH_8086_SYSTEM_H
#define __ARCH_8086_SYSTEM_H

#include <linuxmt/types.h>
#include <linuxmt/init.h>

extern byte_t sys_caps;     /* system capabilities bits*/
extern seg_t membase;       /* start and end segment of available main memory */
extern seg_t memend;

extern unsigned int INITPROC setup_arch(void);
extern void hard_reset_now(void);
extern void apm_shutdown_now(void);

#endif
