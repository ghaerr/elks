#ifndef __ARCH_8086_IRQ_H
#define __ARCH_8086_IRQ_H

#ifdef __KERNEL__
/* irq numbers >= 16 are hardware exceptions/traps or syscall */
#define IDX_SYSCALL     16
#define IDX_DIVZERO     17
#define IDX_NMI         18
#ifdef CONFIG_ARCH_NECV25
#define IDX_NECV25_IBRK 19      /* NEC V25 specific IO Break Exception */
#define NR_IRQS         20      /* = # IRQs plus special indexes above */
#else
#define NR_IRQS         19      /* = # IRQs plus special indexes above */
#endif

#define INT_GENERIC  0  // use the generic interrupt handler (aka '_irqit')
#define INT_SPECIFIC 1  // use a specific interrupt handler

#ifndef __ASSEMBLER__
#include <linuxmt/types.h>

/* irq.c*/
typedef void (* int_proc) (void);  // any INT handler
typedef void (* irq_handler) (int,struct pt_regs *);   // IRQ handler

void do_IRQ(int,struct pt_regs *);
int request_irq(int,irq_handler,int hflag);
int free_irq(int irq);

/* irqtab.S */
void _irqit (void);
void int_vector_set (int vect, word_t proc, word_t seg);
void idle_halt(void);

void div0_handler(int irq, struct pt_regs *regs);
void nmi_handler(int irq, struct pt_regs *regs);
#ifdef CONFIG_ARCH_NECV25
void ibrk_handler(int irq, struct pt_regs *regs);
#endif

/* irq-8259.c, irq-8018x.c, irq-necv25.c */
void initialize_irq(void);
void enable_irq(unsigned int);
void disable_irq(unsigned int irq);
int remap_irq(int);
int irq_vector(int irq);

/* softirq.c */
/* BH handlers, run in increasing numeric order */
enum {
    NETWORK_BH = 0,
    TIMER_BH = 1,
    SERIAL_BH,          /* unused, handled by timer_bh */
    MAX_SOFTIRQ
};
extern unsigned int bh_active;
extern void (*bh_base[MAX_SOFTIRQ])(void);
#define init_bh(nr, routine)    { bh_base[nr] = routine; }
#define mark_bh(nr)             { bh_active |= 1 << (nr);  }
void do_bottom_half(void);

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

/* atomic access of 32-bit volatile jiffies */
#define jiffies()                           \
    __extension__ ({                        \
        unsigned long v;                    \
        asm volatile ("pushfw\n"            \
                      "cli\n"               \
                        : /* no input */    \
                        : /* no output */   \
                        : "memory");        \
        v = jiffies;                        \
        asm volatile ("popfw\n"             \
                        : /* no input */    \
                        : /* no output */   \
                        : "memory");        \
        v; })

/* set stack pointer */
#define setsp(newsp)                    \
        asm volatile ("mov %%ax,%%sp"   \
            : /* no output */           \
            :"a" ((unsigned short)(newsp)) \
            :"memory")

/* breakpoint interrupt */
#define int3()                  \
        asm volatile ("int $3\n"\
            : /* no output */   \
            : /* no input */)

#ifdef CONFIG_ARCH_SWAN
/* acknowledge interrupt */
#define ack_irq(i)              \
        asm volatile ("outb %0, $0xB6\nsti\n"   \
            : /* no output */   \
            :"Ral" ((unsigned char) (1 << (i))) \
            :"memory")
#endif

#endif
