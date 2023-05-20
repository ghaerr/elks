#pragma once

#include <stdint.h>

typedef struct {
  uint8_t  packet_length;
  uint8_t  command_id;
  uint8_t  data[0];
} __attribute__((packed)) CommandHeader;


typedef struct {
  uint32_t read_addr;
  uint8_t  read_len;
} __attribute__((packed)) CommandRead;


typedef struct {
  uint32_t start_addr;
  uint32_t end_addr;
} __attribute__((packed)) CommandCRC;


typedef struct {
  uint32_t write_addr;
  uint8_t  data[0];
} __attribute__((packed)) CommandWrite;


typedef struct {
  uint32_t start_addr;
  uint32_t end_addr;
} __attribute__((packed)) CommandEraseSector;


#define COMMAND_LIST  \
  ENTRY(ping)         \
  ENTRY(identify)     \
  ENTRY(read)         \
  ENTRY(crc)          \
  ENTRY(write)        \
  ENTRY(erase_sector) \
  ENTRY(erase_chip)


#define ENTRY(x) CMD_ ## x,
typedef enum {
  COMMAND_LIST
  CMD_COUNT
} Command;
#undef ENTRY
