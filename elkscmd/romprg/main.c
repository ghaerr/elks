#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "arch/io.h"
#include "arch/8018x.h"

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


int main (int argc, char **argv)
{
    printf("flash program, build on %s\n\n", __TIMESTAMP__);
    fflush(stdout);

    asm("cli");

    // switch to a faster speed
    outw(0x8000 | baudrate_compares[15], PCB_B0CMP); // 38400bps

    for(;;)
    {
        read_and_exec_command();
    }
    
    asm("ljmp $0xf0000, $0xfff0");
    
    return 0;
}
