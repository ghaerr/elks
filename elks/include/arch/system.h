#ifndef __ARCH_8086_SYSTEM_H
#define __ARCH_8086_SYSTEM_H

#include <linuxmt/types.h>
#include <linuxmt/init.h>

extern byte_t sys_caps;		/* system capabilities bits*/

extern void arch_setuptasks(void);
extern void INITPROC setup_arch(seg_t *,seg_t *);

#endif
