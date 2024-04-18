#include "flash.h"
#include <string.h>

#define _MK_FP(seg,off)	((void __far *)((((unsigned long)(seg)) << 16) | (off)))
#define LINEAR(addr32)  _MK_FP(addr32 >> 4, addr32 & 0x0F)

void flash_load(uint32_t addr, uint8_t data)
{
    volatile char __far *ptr = LINEAR((0x80000 | addr));
    *ptr = data;
}

uint8_t flash_read(uint32_t addr)
{
    volatile char __far *ptr = LINEAR((0x80000 | addr));
    return *ptr;
}


void flash_attention(void) {
  flash_load(0x5555, 0xAA);
  flash_load(0x2AAA, 0x55);
}


void flash_exec_function(uint8_t func) {
  flash_attention();
  flash_load(0x5555, func);
}


void flash_enter_software_id(void) {
  flash_exec_function(0x90);
}


void flash_leave_software_id(void) {
  flash_exec_function(0xF0);
}


uint8_t flash_read_manufacturer_id(void) {
  return flash_read(0x0000);
}


uint8_t flash_read_device_id(void) {
  return flash_read(0x0001);
}


void flash_wait_dq7(uint8_t addr, uint8_t value) {
  for(;;) {
    if((flash_read(addr) & 0x80) == value) {
      return;
    }
  }
}


void flash_write_byte(uint32_t addr, uint8_t byte) {
  flash_exec_function(0xA0);
  flash_load(addr, byte);
  //flash_wait_dq7(addr, byte & 0x80);
  asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
}


void flash_erase_sector(uint32_t addr) {
  flash_exec_function(0x80);
  flash_attention();
  flash_load(addr, 0x30);
  flash_wait_dq7(addr & ~(0x7FF), 0x80);
}


void flash_erase_chip(void) {
  flash_exec_function(0x80);
  flash_exec_function(0x10);
  flash_wait_dq7(0, 0x80);
}

