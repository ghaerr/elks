#ifndef LX86_ARCH_SYSTEM_H
#define LX86_ARCH_SYSTEM_H

#include <linuxmt/types.h>

#ifdef __LINT__
#define __ptasks int
#endif

void arch_setuptasks(void);
void load_regs(__ptasks task);
void save_regs(__ptasks task);
void setup_arch(void);

extern int arch_cpu;

#ifdef __LINT__
#undef __ptasks
#endif

#endif
