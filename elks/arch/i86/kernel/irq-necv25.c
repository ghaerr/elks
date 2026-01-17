/*
 * NEC V25 Integrated Interrupt Controller Unit
 * Low level interrupt handling for the NECV25 platforms
 *
 * This file contains code used for the embedded NEC V25 family only.
 * Based on Santiago Hormazabal irq-8018x.c
 *
 * 24 Oct. 2025 swausd
 */

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <arch/ports.h>
#include <arch/necv25.h>
#include <arch/irq.h>


void initialize_irq(void)    // see irq-necv25asm.S
{

// Disable all interrupts and
// set desired irq priorities as this has to be handled in the group base register

    __extension__ ({                   \
        asm volatile (                 \
            ".include \"../../../include/arch/necv25.inc\"                                                                 \n"\
            "push  %%ds                                                                                                    \n"\
            "movw  $NEC_HW_SEGMENT, %%bx           // load DS to access memmory mapped CPU registers                       \n"\
            "movw  %%bx, %%ds                                                                                              \n"\
            "pushf                                 // save iqr status and disable all interrupts                           \n"\
            "cli                                                                                                           \n"\
            "movb  $(0x55), %%ds:(INTM)            // INTM:  Interrupt Mode high/low edge trigger for INTP0..3/NMI         \n"\
            "movb  $(IRQMSK+IRQPRI5), %%ds:(EXIC0) // EXIC0: External interrupt 0, irq prio 5                              \n"\
            "movb  $(IRQMSK+IRQPRID), %%ds:(EXIC1) // EXIC1: External interrupt 1                                          \n"\
            "movb  $(IRQMSK+IRQPRID), %%ds:(EXIC2) // EXIC2: External interrupt 2                                          \n"\
            "movb  $(IRQMSK+IRQPRI6), %%ds:(SEIC0) // SEIC0: Serial error interrupt request control register 0, irq prio 6 \n"\
            "movb  $(IRQMSK+IRQPRID), %%ds:(SRIC0) // SRIC0: Serial reception interrupt request control register 0         \n"\
            "movb  $(IRQMSK+IRQPRID), %%ds:(STIC0) // STIC0: Serial transmission interrupt request control register 0      \n"\
            "movb  $0x00, %%ds:(TXB0)              // TXB0:  restart transmitter, so that ready waiting funtions           \n"\
            "movb  $(IRQMSK+IRQPRI6), %%ds:(SEIC1) // SEIC1: Serial error interrupt request control register 1, irq prio 6 \n"\
            "movb  $(IRQMSK+IRQPRID), %%ds:(SRIC1) // SRIC1: Serial reception interrupt request control register 1         \n"\
            "movb  $(IRQMSK+IRQPRID), %%ds:(STIC1) // STIC1: Serial transmission interrupt request control register 1      \n"\
            "movb  $0x00, %%ds:(TXB1)              // TXB1:  restart transmitter, so that ready waiting funtions           \n"\
            "movb  $(IRQMSK+IRQPRI7), %%ds:(TMIC0) // TMIC0: Timer unit interrupt request control register 0, irq prio 7   \n"\
            "movb  $(IRQMSK+IRQPRID), %%ds:(TMIC1) // TMIC1: Timer unit interrupt request control register 1               \n"\
            "movb  $(IRQMSK+IRQPRID), %%ds:(TMIC2) // TMIC2: Timer unit interrupt request control register 2               \n"\
            "movb  $(IRQMSK+IRQPRI4), %%ds:(DIC0)  // DIC0:  DMA interrupt 0, irq prio 4                                   \n"\
            "movb  $(IRQMSK+IRQPRID), %%ds:(DIC1)  // DIC1:  DMA interrupt 1                                               \n"\
            "movb  $(IRQMSK+IRQPRID), %%ds:(TBIC)  // TBIC:  Time base interrupt, fixed irq prio 7                         \n"\
            "popf                                  // restore flags and irq status                                         \n"\
            "pop   %%ds                                                                                                    \n"\
            :                    \
            :                    \
            : "bx", "memory" );  \
    });
}

struct irq_logical_map {
    unsigned int  irq;            /* logical IRQ from ELKS */
    unsigned int  pcb_register;   /* corresponding NECV25 interrupt control register */
    int           irq_vector;     /* corresponding NECV25 CPU IRQ Vector number */
}
logical_map[] =
{
    { TIMER_IRQ,    NEC_TMIC2, NEC_INTTU2 }, // Timmer 1 IRQ
    { UART1_IRQ_RX, NEC_SRIC1, NEC_INTSR1 }, // Serial 1 RX IRQ
#ifdef CONFIG_FAST_IRQ2_NECV25
    { UART2_IRQ_RX, NEC_SRIC0, NEC_INTSR0 }, // Serial 0 RX IRQ
#endif
#ifdef UNUSED
    { UART1_IRQ_TX, NEC_STIC1, NEC_INTST1 }, // Serial 1 TX IRQ
    { UART2_IRQ_TX, NEC_STIC0, NEC_INTST0 }, // Serial 0 TX IRQ
#endif
    { NE2K_IRQ,     NEC_EXIC2, NEC_INTP2 }, // NE2000 NIC, map NIC IRQ 2 to NEC V25 INTP2
};

struct irq_logical_map* get_from_logical_irq(unsigned int irq)
{
    size_t i;
    for (i = 0; i < (sizeof(logical_map)/sizeof(logical_map[0])); ++i)
    {
        if (logical_map[i].irq == irq)
        {
            return &logical_map[i];
        }
    }

    return NULL;
}

void enable_irq(unsigned int irq)
{
    struct irq_logical_map* map;
    map = get_from_logical_irq(irq);

    if (map) // unmask irq bit
    {
       __extension__ ({                   \
           asm volatile (                 \
               ".include \"../../../include/arch/necv25.inc\"                                  \n"\
               "push  %%ds                                                                     \n"\
               "movw  $NEC_HW_SEGMENT, %%bx  // load DS to access memmory mapped CPU registers \n"\
               "movw  %%bx, %%ds                                                               \n"\
               ".word 0x1a0f                 // NEC V25 special: clr1 (%%si), 0x07 set IRQFLAG \n"\
               ".word 0x0704                                                                   \n"\
               ".word 0x1a0f                 // NEC V25 special: clr1 (%%si), 0x07 clr IRQMSK  \n"\
               ".word 0x0604                                                                   \n"\
               "pop   %%ds                                                                     \n"\
               :                          \
               : "S"   (map->pcb_register)\
               : "bx", "memory" );        \
       });
    }
}

int remap_irq(int irq)
{
    /* no remaps */
    return irq;
}

void disable_irq(unsigned int irq)
{
    struct irq_logical_map* map;
    map = get_from_logical_irq(irq);

    if (map) // mask irq bit
    {
       __extension__ ({                   \
           asm volatile (                 \
               ".include \"../../../include/arch/necv25.inc\"                                  \n"\
               "push  %%ds                                                                     \n"\
               "movw  $NEC_HW_SEGMENT, %%bx  // load DS to access memmory mapped CPU registers \n"\
               "movw  %%bx, %%ds                                                               \n"\
               ".word 0x1c0f                 // NEC V25 special: set1 (%%si), 0x06 set IRQMSK  \n"\
               ".word 0x0604                                                                   \n"\
               "pop   %%ds                                                                     \n"\
               :                          \
               : "S"   (map->pcb_register)\
               : "bx", "memory" );        \
       });
    }
}

// Get interrupt vector from IRQ
int irq_vector(int irq)
{
    struct irq_logical_map* map;

    map = get_from_logical_irq(irq);

    if (map) {
        return map->irq_vector;
    }

    return -EINVAL;
}
