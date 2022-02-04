#ifndef __ARCH_8086_SYSTEM_H
#define __ARCH_8086_SYSTEM_H

#include <linuxmt/types.h>
#include <linuxmt/init.h>

extern byte_t sys_caps;		/* system capabilities bits*/

extern void INITPROC setup_arch(seg_t *,seg_t *);
extern void hard_reset_now(void);
extern void apm_shutdown_now(void);

#endif
