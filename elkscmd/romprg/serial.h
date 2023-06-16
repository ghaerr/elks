#pragma once

#include <stdint.h>

int open_serial(char const* device);
void uart_write_byte(uint8_t byte);
uint8_t uart_read_byte(void);
void close_serial(void);
