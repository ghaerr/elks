#include "serial.h"
#include "arch/io.h"
#include <linuxmt/config.h>

#ifndef CONFIG_ARCH_NECV25
#include "arch/8018x.h"

uint8_t uart_read_byte(void) {
  while((inw(PCB_S0STS) & 0x40) == 0);
  return inb(PCB_R0BUF);
}

void uart_write_byte(uint8_t byte) {
    while((inw(PCB_S0STS) & 8) == 0);
    outb(byte, PCB_T0BUF);
}

#else
#include <arch/necv25.h>

uint8_t uart_read_byte(void) {
    uint8_t byte;

    __extension__ ({                   \
        asm volatile (                 \
            ".include \"../../elks/include/arch/necv25.inc\"                                             \n"\
            "push  %%ds                                                                                  \n"\
            "movw  $NEC_HW_SEGMENT, %%bx              // load DS to access memmory mapped CPU registers  \n"\
            "movw  %%bx, %%ds                                                                            \n"\
            "xorw  %%ax, %%ax                                                                            \n"\
            "1:                                                                                          \n"\
            "testb $IRQFLAG, %%ds:SRIC(%%si)          // SRIC: char ready?                               \n"\
            "jz    1b                                 // no                                              \n"\
            "movb  %%ds:(%%si), %%al                  // RXB:  get char                                  \n"\
            "movb  $(IRQMSK+IRQPRID), %%ds:SRIC(%%si) // SRIC: clear Rx ready flag                       \n"\
            "pop   %%ds                                                                                  \n"\
            : "=Ral" (byte)      \
            : "S"    (NEC_RXB1)  \
            : "bx", "memory" );  \
    });
    
    return byte;
}

	
void uart_write_byte(uint8_t byte) {
    __extension__ ({                   \
        asm volatile (                 \
            ".include \"../../elks/include/arch/necv25.inc\"                                            \n"\
            "push  %%ds                                                                                 \n"\
            "movw  $NEC_HW_SEGMENT, %%bx              // load DS to access memmory mapped CPU registers \n"\
            "movw  %%bx, %%ds                                                                           \n"\
            "1:                                                                                         \n"\
            "testb $IRQFLAG, %%ds:STIC(%%si)          // STIC: wait for Tx ready                        \n"\
            "jz    1b                                                                                   \n"\
            "movb  $(IRQMSK+IRQPRID), %%ds:STIC(%%si) // STIC: clear Tx ready flag                      \n"\
            "movb  %%al, %%ds:TXB(%%si)               // TXB: send char                                 \n"\
            "pop   %%ds                                                                                 \n"\
            :                          \
            : "S"   (NEC_RXB1),        \
              "Ral" (byte)             \
            : "bx", "memory" );        \
    });
}
#endif
