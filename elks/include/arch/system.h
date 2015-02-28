#ifndef LX86_ARCH_SYSTEM_H
#define LX86_ARCH_SYSTEM_H

#include <linuxmt/types.h>

extern int arch_cpu;

extern void arch_setuptasks(void);
extern void setup_arch(seg_t *,seg_t *);

extern int in_group_p(gid_t);

#endif
