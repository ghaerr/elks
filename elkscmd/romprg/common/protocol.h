#pragma once

#include <stdint.h>


#define ERROR_NONE      0
#define ERROR_STUFFING  1
#define ERROR_CRC       2
#define ERROR_COMMAND   3
#define ERROR_ADDR      4
#define ERROR_LENGTH    5
#define ERROR_TEST_MODE 6

#define ERROR_LOCAL 0x10000000UL

uint8_t protocol_read_packet(unsigned buffer_size, uint8_t *buffer);
void protocol_write_packet(uint8_t data_len, uint8_t const* data);
