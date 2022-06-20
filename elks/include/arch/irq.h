#ifndef __ARCH_8086_IRQ_H
#define __ARCH_8086_IRQ_H

#include <linuxmt/types.h>

/* irq.c*/

#define INT_GENERIC  0  // use the generic interrupt handler (aka '_irqit')
#define INT_SPECIFIC 1  // use a specific interrupt handler

typedef void (* int_proc) (void);  // any INT handler
typedef void (* irq_handler) (int,struct pt_regs *);   // IRQ handler

void do_IRQ(int,void *);
int request_irq(int,irq_handler,int hflag);
void free_irq(unsigned int irq);
void int_vector_set (int vect, int_proc proc, int seg);
void _irqit (void);


/* irq-8259.c*/
void init_irq(void);
void enable_irq(unsigned int);
void disable_irq(unsigned int irq);
int remap_irq(int);
int irq_vector (int irq);

void idle_halt(void);

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
