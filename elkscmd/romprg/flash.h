#pragma once

#include <stdint.h>

void flash_set_address(uint32_t addr);
void flash_set_direction(uint8_t inout);
void flash_set_data(uint8_t data);
uint8_t flash_get_data(void);
int flash_set_output_enable(uint8_t enable);
void flash_set_write_enable(uint8_t enable);
void flash_trigger_write(void);
void flash_set_read_mode(void);
void flash_set_write_mode(void);
void flash_load(uint32_t addr, uint8_t data);
uint8_t flash_read(uint32_t addr);
void flash_attention(void);
void flash_exec_function(uint8_t func);
void flash_enter_software_id(void);
void flash_leave_software_id(void);
uint8_t flash_read_manufacturer_id(void);
uint8_t flash_read_device_id(void);
void flash_wait_dq7(uint8_t addr, uint8_t value);
void flash_write_byte(uint32_t addr, uint8_t byte);
void flash_erase_sector(uint32_t addr);
void flash_erase_chip(void);
