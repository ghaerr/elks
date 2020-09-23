#ifndef __ARCH_8086_SYSTEM_H
#define __ARCH_8086_SYSTEM_H

#include <linuxmt/types.h>
#include <linuxmt/init.h>

extern byte_t arch_cpu;  // processor number (from setup data)

extern void arch_setuptasks(void);
extern void INITPROC setup_arch(seg_t *,seg_t *);

extern int in_group_p(gid_t);

#endif
