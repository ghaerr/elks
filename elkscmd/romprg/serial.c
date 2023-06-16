#include "serial.h"
#include "arch/io.h"
#include "arch/8018x.h"

uint8_t uart_read_byte(void) {
  while((inw(PCB_S0STS) & 0x40) == 0);
  return inb(PCB_R0BUF);
}

void uart_write_byte(uint8_t byte) {
    while((inw(PCB_S0STS) & 8) == 0);
    outb(byte, PCB_T0BUF);
}