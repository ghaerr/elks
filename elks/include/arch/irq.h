#ifndef __ARCH_8086_IRQ_H
#define __ARCH_8086_IRQ_H

#ifdef __KERNEL__
/* irq numbers >= 16 are hardware exceptions/traps or syscall */
#define IDX_SYSCALL     16
#define IDX_DIVZERO     17
#define NR_IRQS         18      /* = # IRQs plus special indexes above */

#define INT_GENERIC  0  // use the generic interrupt handler (aka '_irqit')
#define INT_SPECIFIC 1  // use a specific interrupt handler

#ifndef __ASSEMBLER__
#include <linuxmt/types.h>

/* irq.c*/
typedef void (* int_proc) (void);  // any INT handler
typedef void (* irq_handler) (int,struct pt_regs *);   // IRQ handler

void do_IRQ(int,struct pt_regs *);
void div0_handler(int, struct pt_regs *);
int request_irq(int,irq_handler,int hflag);
int free_irq(int irq);

/* irqtab.S */
void _irqit (void);
void int_vector_set (int vect, word_t proc, word_t seg);
void div0_handler_panic(void);

/* irq-8259.c, irq-8018x.c*/
void initialize_irq(void);
void enable_irq(unsigned int);
void disable_irq(unsigned int irq);
int remap_irq(int);
int irq_vector(int irq);

void idle_halt(void);
#endif /* __ASSEMBLER__ */
#endif /* __KERNEL__ */

#ifdef __ia16__
#define save_flags(x)                   \
        asm volatile ("pushfw\n"        \
                      "popw %0\n"       \
                        :"=r" (x)       \
                        : /* no input */\
                        :"memory")

#define restore_flags(x)                \
        asm volatile ("pushw %0\n"      \
                      "popfw\n"         \
                        : /* no output*/\
                        :"r" (x)        \
                        :"memory")

// From Linux arch/x86/include/asm/irqflags.h
// Note the memory barrier for the compiler

/* disable interrupts */
#define clr_irq()               \
        asm volatile ("cli\n"   \
            : /* no output */   \
            : /* no input */    \
            :"memory")

/* enable interrupts */
#define set_irq()               \
        asm volatile ("sti\n"   \
            : /* no output */   \
            : /* no input */    \
            :"memory")

/* interrupt 0Fh (irq 7) */
#define int0F()                 \
        asm volatile ("int $0x0F\n"   \
            : /* no output */   \
            : /* no input */    \
            :"memory")
#endif

#endif
