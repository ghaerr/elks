#ifndef LX86_ARCH_SYSTEM_H
#define LX86_ARCH_SYSTEM_H

#include <linuxmt/types.h>

extern int arch_cpu;

void arch_setuptasks(void);
void setup_arch(void);

/* LINT complains about the following two entries, but I have no idea
 * what the correct prototype is as __ptaksks doesn't occur elsewhere.
 * The following definition is very much a stop-gap measure to silence
 * LINT and is otherwise unused.
 */

#ifdef __LINT__
#define __ptasks int
#endif

void load_regs(__ptasks);
void save_regs(__ptasks);

#ifdef __LINT__
#undef __ptasks
#endif

#endif
