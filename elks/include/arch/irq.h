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

#ifdef __ia16__
#define save_flags(x)                   \
        asm volatile("pushfw\n"         \
                     "\tpopw %0\n"      \
                        :"=r" (x)       \
                        : /* no input */\
                        :"memory")

#define restore_flags(x)                \
        asm volatile("pushw %0\n"       \
                     "\tpopfw\n"        \
                        : /* no output*/\
                        :"r" (x)        \
                        :"memory")

// From Linux arch/x86/include/asm/irqflags.h
// Note the memory barrier for the compiler

/* disable interrupts */
#define clr_irq()       \
        asm volatile ("cli": : :"memory")

/* enable interrupts */
#define set_irq()       \
        asm volatile ("sti": : :"memory")
#endif

#endif
