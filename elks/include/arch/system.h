#ifndef LX86_ARCH_SYSTEM_H
#define LX86_ARCH_SYSTEM_H

#include <linuxmt/types.h>

extern int arch_cpu;

extern void arch_setuptasks(void);
extern void setup_arch(seg_t *,seg_t *);

/* LINT complains about the following two entries, but I have no idea
 * what the correct prototype is as __ptaksks doesn't occur elsewhere.
 * The following definition is very much a stop-gap measure to silence
 * LINT and is otherwise unused.
 */

#ifdef S_SPLINT_S
#define __ptasks int
#endif

extern void load_regs(__ptasks);
extern void save_regs(__ptasks);

#ifdef S_SPLINT_S
#undef __ptasks
#endif

extern int in_group_p(gid_t);
extern int get_unused_fd(void);

#endif
