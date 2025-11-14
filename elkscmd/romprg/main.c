#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "arch/io.h"

#include <linuxmt/types.h>
#include <linuxmt/config.h>

#include "flash.h"
#include "commands.h"
#include "common/protocol.h"
#include "serial.h"

void read_and_exec_command(void) {
  uint8_t buffer[140];

  uint8_t err = protocol_read_packet(sizeof(buffer), buffer);
  if(err != ERROR_NONE) {
    command_generate_error_reply(err);
  } else {
    command_execute(buffer);
  }
}


#ifndef CONFIG_ARCH_NECV25
#include "arch/8018x.h"

static const unsigned int baudrate_compares[] = {
    0,  /*  0 = B0      */
    (CONFIG_8018X_FCPU * 1000000UL / (50UL * 8UL)) - 1,     /*  1 = B50     */
    (CONFIG_8018X_FCPU * 1000000UL / (75UL * 8UL)) - 1,     /*  2 = B75     */
    (CONFIG_8018X_FCPU * 1000000UL / (110UL * 8UL)) - 1,    /*  3 = B110    */
    (CONFIG_8018X_FCPU * 1000000UL / (134UL * 8UL)) - 1,    /*  4 = B134    */
    (CONFIG_8018X_FCPU * 1000000UL / (150UL * 8UL)) - 1,    /*  5 = B150    */
    (CONFIG_8018X_FCPU * 1000000UL / (200UL * 8UL)) - 1,    /*  6 = B200    */
    (CONFIG_8018X_FCPU * 1000000UL / (300UL * 8UL)) - 1,    /*  7 = B300    */
    (CONFIG_8018X_FCPU * 1000000UL / (600UL * 8UL)) - 1,    /*  8 = B600    */
    (CONFIG_8018X_FCPU * 1000000UL / (1200UL * 8UL)) - 1,   /*  9 = B1200   */
    (CONFIG_8018X_FCPU * 1000000UL / (1800UL * 8UL)) - 1,   /* 10 = B1800   */
    (CONFIG_8018X_FCPU * 1000000UL / (2400UL * 8UL)) - 1,   /* 11 = B2400   */
    (CONFIG_8018X_FCPU * 1000000UL / (4800UL * 8UL)) - 1,   /* 12 = B4800   */
    (CONFIG_8018X_FCPU * 1000000UL / (9600UL * 8UL)) - 1,   /* 13 = B9600   */
    (CONFIG_8018X_FCPU * 1000000UL / (19200UL * 8UL)) - 1,  /* 14 = B19200  */
    (CONFIG_8018X_FCPU * 1000000UL / (38400UL * 8UL)) - 1,  /* 15 = B38400  */
    (CONFIG_8018X_FCPU * 1000000UL / (57600UL * 8UL)) - 1,  /* 16 = B57600  */
    (CONFIG_8018X_FCPU * 1000000UL / (115200UL * 8UL)) - 1, /* 17 = B115200 */
    (CONFIG_8018X_FCPU * 1000000UL / (230400UL * 8UL)) - 1, /* X3 = B230400 */
    (CONFIG_8018X_FCPU * 1000000UL / (460800UL * 8UL)) - 1, /* X4 = B460800 */
    (CONFIG_8018X_FCPU * 1000000UL / (500000UL * 8UL)) - 1, /* X5 = B500000 */
};

void set_baud_rate(uint8_t idx) {
    outw(0x8000 | baudrate_compares[idx], PCB_B0CMP); // 38400bps
}

#else
#include <arch/necv25.h>

static const struct baud_tab {
    unsigned char count; /* NEC_BRGx counter              */
    unsigned char n;     /* NEC_SCCx prescaler selector n */
} baud_setup[] = {                                          /*         counter n */
                                                            /*         counter n */
    {0,                                          15},       /*  0 = B0       0 f */
    {(CONFIG_NECV25_FCPU / (    50UL * 2UL * 2048UL)), 10}, /*  1 = B50     72 a */
    {(CONFIG_NECV25_FCPU / (    75UL * 2UL * 2048UL)), 10}, /*  2 = B75     48 a */
    {(CONFIG_NECV25_FCPU / (   110UL * 2UL * 1024UL)),  9}, /*  3 = B110    65 9 */
    {(CONFIG_NECV25_FCPU / (   134UL * 2UL * 1024UL)),  9}, /*  4 = B134    54 9 */
    {(CONFIG_NECV25_FCPU / (   150UL * 2UL * 1024UL)),  9}, /*  5 = B150    48 9 */
    {(CONFIG_NECV25_FCPU / (   200UL * 2UL *  512UL)),  8}, /*  6 = B200    72 8 */
    {(CONFIG_NECV25_FCPU / (   300UL * 2UL *  512UL)),  8}, /*  7 = B300    48 8 */
    {(CONFIG_NECV25_FCPU / (   600UL * 2UL *  256UL)),  7}, /*  8 = B600    48 7 */
    {(CONFIG_NECV25_FCPU / (  1200UL * 2UL *  128UL)),  6}, /*  9 = B1200   48 6 */
    {(CONFIG_NECV25_FCPU / (  1800UL * 2UL *  128UL)),  6}, /* 10 = B1800   32 6 */
    {(CONFIG_NECV25_FCPU / (  2400UL * 2UL *   64UL)),  5}, /* 11 = B2400   48 5 */
    {(CONFIG_NECV25_FCPU / (  4800UL * 2UL *   16UL)),  3}, /* 12 = B4800   96 3 */
    {(CONFIG_NECV25_FCPU / (  9600UL * 2UL *   16UL)),  3}, /* 13 = B9600   48 3 */
    {(CONFIG_NECV25_FCPU / ( 19200UL * 2UL *    8UL)),  2}, /* 14 = B19200  48 2 */
    {(CONFIG_NECV25_FCPU / ( 38400UL * 2UL *    4UL)),  1}, /* 15 = B38400  48 1 */
    {(CONFIG_NECV25_FCPU / ( 57600UL * 2UL *    2UL)),  0}, /* 16 = B57600  64 0 */
    {(CONFIG_NECV25_FCPU / (115200UL * 2UL *    2UL)),  0}, /* 17 = B115200 32 0 */
    {(CONFIG_NECV25_FCPU / (230400UL * 2UL *    2UL)),  0}  /*  0 = B230400 16 0 */
};

void set_baud_rate(uint8_t idx) {
   __extension__ ({                   \
      asm volatile (                 \
          ".include \"../../elks/include/arch/necv25.inc\"                                                              \n"\
          "push   %%ds                                                                                                  \n"\
          "movw   $NEC_HW_SEGMENT, %%bx                     // load DS to access memmory mapped CPU registers           \n"\
          "movw   %%bx, %%ds                                                                                            \n"\
          "pushf                                            // save iqr status and disable all interrupts               \n"\
          "cli                                                                                                          \n"\
          "movb   $(NOPRTY+CL8+SL1), %%ds:SCM(%%si)         // SCM: Rx/Tx disable, 8 data, 1 stop bit, no parity, async \n"\
          "movb   %%al, %%ds:SCC(%%si)                      // SCC: set prescaler                                       \n"\
          "movb   %%ah, %%ds:BRG(%%si)                      // BRG: set counter                                         \n"\
          "movb   $(TXE+RXE+NOPRTY+CL8+SL1), %%ds:SCM(%%si) // SCM: Rx/Tx enable, 8 data, 1 stop bit, no parity, async  \n"\
          "popf                                             // restore flags and irq status                             \n"\
          "pop    %%ds                                                                                                  \n"\
          :                               \
          : "S"   (NEC_RXB1),             \
            "Ral" (baud_setup[idx].n),    \
            "Rah" (baud_setup[idx].count) \
          : "bx", "memory" );             \
   });
}
#endif


int main (int argc, char **argv)
{
    printf("Flash program, build on %s\n\n", __TIMESTAMP__);

#ifdef CONFIG_ARCH_NECV25
    printf("Attention: On NEC V25 0xf:fe00 - 0xf:ffff cannot be programmed with romprg!\n");
    printf("           This is the MCU internal RAM/IO address space.\n\n");
#endif

    printf("Exit the terminal emulation now and start the host programmer client...\n\n");
    


    fflush(stdout);

    asm("cli");

    // switch to a faster baud rate for data tranfere
    // Has to match with PROGRAMMER_BAUD in "version.h"
    set_baud_rate(17);

    for(;;)
    {
        read_and_exec_command();
    }
    
    asm("ljmp $0xf0000, $0xfff0");
    
    return 0;
}
