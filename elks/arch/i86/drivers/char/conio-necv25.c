/*
 * conio API for NEC V25 Headless Console
 *
 * This file contains code used for the embedded NEC V25 family only.
 * Not used now, only for reference.
 *
 * 29. Oct. 2025 swausd
 */

#include <linuxmt/config.h>
#include <arch/io.h>
#include <arch/irq.h>
#include <arch/necv25.h>
#include "conio.h"


#ifdef UNUSED

/* initialize */
void conio_init(void)
{
}

/*
 * Poll for console input available.
 * Return nonzero character received else 0 if none ready.
 * Called approximately every ~8/100 seconds.
 */
int conio_poll(void)
{
    __extension__ ({                   \
        asm volatile (                 \
            ".include \"../../../../include/arch/necv25.inc\"                                            \n"\
            "push  %%ds                                                                                  \n"\
            "movw  $NEC_HW_SEGMENT, %%bx              // load DS to access memmory mapped CPU registers  \n"\
            "movw  %%bx, %%ds                                                                            \n"\
            "xorw  %%ax, %%ax                                                                            \n"\
            "testb $IRQFLAG, %%ds:SRIC(%%si)          // SRIC: char ready?                               \n"\
            "jz    1f                                 // no                                              \n"\
            "movb  %%ds:(%%si), %%al                  // RXB:  get char                                  \n"\
            "movb  $$IRQMSK+IRQPRID, %%ds:SRIC(%%si)  // SRIC: clear Rx ready flag                       \n"\
            "1:                                                                                          \n"\
            "pop   %%ds                                                                                  \n"\
            :                          \
            : "S"   (NEC_RXB1)         \
            : "ax", "bx", "memory" );  \
    });
}
    
void conio_putc(byte_t c)
{
    __extension__ ({                   \
        asm volatile (                 \
            ".include \"../../../../include/arch/necv25.inc\"                                         \n"\
            "push  %%ds                                                                               \n"\
            "movw  $NEC_HW_SEGMENT, %%bx            // load DS to access memmory mapped CPU registers \n"\
            "movw  %%bx, %%ds                                                                         \n"\
            "1:                                                                                       \n"\
            "testb $IRQFLAG, %%ds:STIC(%%si)        // STIC: wait for Tx ready                        \n"\
            "jz    1b                                                                                 \n"\
            "pushf                                  // save iqr status and disable all interrupts     \n"\
            "cli                                                                                      \n"\
            "movb  $IRQMSK+IRQPRID, %%ds:STIC(%%si) // STIC: clear Tx ready flag                      \n"\
            "movb  %%al, %%ds:TXB(%%si)             // TXB: send char                                 \n"\
            "popf                                   // restore flags and irq status                   \n"\
            "pop   %%ds                                                                               \n"\
            :                          \
            : "S"   (NEC_RXB1),        \
              "Ral" (c)                \
            : "bx", "memory" );        \
    });
}


#endif
