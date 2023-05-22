#pragma once

#include <stdint.h>

typedef struct {
  uint8_t error_code;
  uint8_t firmware_version_major;
  uint8_t firmware_version_minor;
  uint8_t address_bits;
  uint8_t max_write_length;
} __attribute__((packed)) ReplyPing;


typedef struct {
  uint8_t error_code;
  uint8_t manufacturer_id;
  uint8_t device_id;
} __attribute__((packed)) ReplyIdentify;


typedef struct {
  uint8_t  error_code;
  uint16_t crc;
} __attribute__((packed)) ReplyCRC;
