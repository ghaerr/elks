/*
 * 8018X's Integrated Interrupt Controller Unit
 *
 * This file contains code used for the embedded 8018X family only.
 * 
 * 16 May 21 Santiago Hormazabal
 */

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>

#include <arch/ports.h>
#include <arch/8018x.h>
#include <arch/io.h>
#include <arch/irq.h>

/*
 *	Low level interrupt handling for the X86 8018X
 *	platforms
 */

void init_irq(void)
{
    /* Mask all interrupt sources on the IMASK register */
    outw(0xfd, PCB_IMASK);
}

struct irq_logical_map {
    unsigned int irq;           /* logical IRQ from ELKS */
    unsigned int config_word;   /* config word for the Interrupt control register */
    unsigned int pcb_register;  /* interrupt control register on the PCB */
    int irq_vector;             /* CPU IRQ Vector number */
} logical_map[] = {
    /* 8018x: Timer Interrupt unmasked, priority 7, Interrupt type (vector) 18. */
    { TIMER_IRQ, 0x7, PCB_TCUCON, CPU_VEC_TIMER1 },
    /**
     * 8018x: Enabling Serial Interrupts will enable the RX and TX IRQs
     * at the same time, this is a CPU limitation.
     * Serial Interrupts unmasked, priority 1, Interrupt type (vector) 20 and 21.
     */
    { UART0_IRQ_RX, 0x1, PCB_SCUCON, CPU_VEC_S0_RX },
    { UART0_IRQ_TX, 0x1, PCB_SCUCON, CPU_VEC_S0_TX },
#if CONFIG_8018X_INT0 + 0 > 0
    { CONFIG_8018X_INT0, 0x6, PCB_I0CON, CPU_VEC_INT0 },
#endif
#if CONFIG_8018X_INT1 + 0 > 0
    { CONFIG_8018X_INT1, 0x6, PCB_I1CON, CPU_VEC_INT1 },
#endif
#if CONFIG_8018X_INT2 + 0 > 0
    { CONFIG_8018X_INT2, 0x6, PCB_I2CON, CPU_VEC_INT2 },
#endif
#if CONFIG_8018X_INT3 + 0 > 0 
    { CONFIG_8018X_INT3, 0x6, PCB_I3CON, CPU_VEC_INT3 },
#endif
#if CONFIG_8018X_INT4 + 0 > 0
    { CONFIG_8018X_INT4, 0x6, PCB_I4CON, CPU_VEC_INT4 },
#endif
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
    if (map) {
        outw(map->config_word, map->pcb_register);
    }
}

int remap_irq(int irq)
{
    /* no remaps */
    return irq;
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
