/*
 * NEC V25 Integrated Timer/Counter Unit
 *
 * This file contains code used for the embedded NEC V25 family only.
 *
 * 24. Oct. 2025 swausd
 */

#include <linuxmt/config.h>
#include <arch/io.h>
#include <arch/param.h> /* For the definition of HZ */
#include <arch/necv25.h>


void enable_timer_tick(void)
{
   // Enable Timer 1 for 100Hz = 10 ms ticks.
   unsigned int n = CONFIG_NECV25_FCPU/2UL/6UL/100UL; // = 12288 = 0x3000

   __extension__ ({                 \
      asm volatile (                \
          ".include \"../../../include/arch/necv25.inc\"                                                \n"\
          "push  %%ds                                                                                   \n"\
          "movw  $NEC_HW_SEGMENT, %%bx                // load DS to access memmory mapped CPU registers \n"\
          "movw  %%bx, %%ds                                                                             \n"\
          "pushf                                      // save iqr status and disable all interrupts     \n"\
          "cli                                                                                          \n"\
          "movb  $TSTOP+TCLK6+TMODEI, %%ds:TMC0(%%si) // TMC0: fclk/6, stop timer 0, set intervall mode \n"\
          "movb  $TSTOP+TCLK6, %%ds:TMC1(%%si)        // TMC1: fclk/6, stop timer 1                     \n"\
          "movw  %%ax, %%ds:MD1(%%si)                 // MD1:  set timer 1 modulo register for 10ms     \n"\
          "movb  $TLOAD+TCLK6, %%ds:TMC1(%%si)        // TMC1: fclk/6, start timer 1                    \n"\
          "popf                                       // restore flags and irq status                   \n"\
          "pop   %%ds                                                                                   \n"\
          :                         \
          : "S"   (NEC_TM0),        \
            "a"   (n)               \
          : "bx", "memory" );       \
   });
}

void disable_timer_tick(void)
{
   __extension__ ({                 \
      asm volatile (                \
          ".include \"../../../include/arch/necv25.inc\"                                                \n"\
          "push  %%ds                                                                                   \n"\
          "movw  $NEC_HW_SEGMENT, %%bx                // load DS to access memmory mapped CPU registers \n"\
          "movw  %%bx, %%ds                                                                             \n"\
          "movb  $TSTOP+TCLK6, %%ds:TMC1(%%si)        // TMC1: fclk/6, stop timer 1                     \n"\
          "pop   %%ds                                                                  \n"\
          :                         \
          : "S"   (NEC_TM0)        \
          : "bx", "memory" );       \
   });

}
