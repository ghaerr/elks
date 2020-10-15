#ifndef __ARCH_8086_IRQ_H
#define __ARCH_8086_IRQ_H

#include <linuxmt/types.h>

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);
extern void do_IRQ(int,void *);
extern int request_irq(int,void (*)(int,struct pt_regs *,void *),void *);
extern void free_irq(unsigned int);

#ifdef CONFIG_IDLE_HALT
void idle_halt (void);
#endif

#ifdef __KERNEL__

#ifdef __ia16__
#define save_flags(x) \
__asm__ __volatile__("pushfw\n" \
          "        popw %0\n" \
          :"=r" (x): /* no input */ :"memory")

#define restore_flags(x) \
__asm__ __volatile__("pushw %0\n" \
          "        popfw\n" \
          : /* no output */ :"r" (x):"memory")

// From Linux arch/x86/include/asm/irqflags.h
// Note the memory barrier for the compiler

#define irq_disable(_x) \
	asm volatile ("cli": : :"memory")

#define irq_enable(_x) \
	asm volatile ("sti": : :"memory")

#define clr_irq irq_disable
#define set_irq irq_enable

#endif  // __ia16__

#else  // __KERNEL__

#define save_flags(x)
#define restore_flags(x)
#define clr_irq()
#define set_irq()

#endif  // __KERNEL__

#endif
